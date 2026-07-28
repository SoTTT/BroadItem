#pragma once

#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QString>
#include <QObject>
#include <QDebug>
#include <functional>

#include <broaditem/diagnostics/Diagnostics.h>

namespace BroadItem {

class PropertyContext;

/**
 * @brief 方括号语法糖代理对象。
 *
 * 由 PropertyContext::operator[] 和 BroadItem::operator[] 返回，
 * 支持链式方括号访问（键名与数组下标），最终通过隐式转换读取或通过
 * operator= 写入。路径拼接规则与 property/setProperty 的嵌套路径格式一致。
 */
class PropertyProxy {
public:
    PropertyProxy() = default;

    PropertyProxy(PropertyContext* ctx, const QString& path)
        : m_ctx(ctx), m_path(path) {}

    PropertyProxy operator[](const QString& key) const;
    PropertyProxy operator[](int index) const;
    operator QVariant() const;
    PropertyProxy& operator=(const QVariant& value);

    bool isValid() const { return m_ctx != nullptr; }

private:
    PropertyContext* m_ctx = nullptr;
    QString m_path;
};

/// @brief 属性上下文的抽象基类，为布局元素提供数据绑定。
class PropertyContext {
public:
    virtual ~PropertyContext() = default;

    /**
     * @brief 读取属性值，支持扁平键和嵌套路径。
     * @param name 属性名，可为扁平键（如 "cpu"）或路径（如 "device.cpu"）。
     * @return 属性值，不存在时返回无效 QVariant。
     */
    virtual QVariant property(const QString& name) const = 0;

    /**
     * @brief 判断指定属性是否存在。
     * @param name 属性名或路径。
     * @return 属性存在返回 true，否则 false。
     */
    virtual bool hasProperty(const QString& name) const = 0;

    /**
     * @brief 设置属性值，并触发变更通知。
     * @param name 属性名（扁平键，不支持路径）。
     * @param value 属性值；无效或 null 值表示移除属性（策略相关）。
     */
    virtual void setProperty(const QString& name, const QVariant& value) = 0;

    /** @brief 属性变更回调类型。 */
    using OnChanged = std::function<void(const QString& name, const QVariant& value)>;

    /**
     * @brief 注册属性变更回调。
     * @param cb 回调函数，参数为属性名和新值。
     */
    void setOnChanged(OnChanged cb)
    {
        m_onChanged = std::move(cb);
    }

    /**
     * @brief 方括号语法糖入口。
     * @param key 首段键名。
     * @return PropertyProxy 代理对象，支持链式方括号访问。
     */
    PropertyProxy operator[](const QString& key) const
    {
        return PropertyProxy(const_cast<PropertyContext*>(this), key);
    }

protected:
    /**
     * @brief 触发属性变更通知，由子类在 setProperty 中调用。
     * @param name 变更的属性名。
     * @param value 新的属性值。
     */
    void notifyChanged(const QString& name, const QVariant& value) const
    {
        if (m_onChanged)
            m_onChanged(name, value);
    }

    /**
     * @brief 纯嵌套路径遍历引擎。
     *
     * 从已解析的首段值出发，递进处理后续的 .key 和 [n] 操作。
     * 每步执行类型断言，失败时报告结构化诊断（BI-R 系列）并返回无效值。
     *
     * @param current 首段 key 解析后的值（已通过首段查找获得）。
     * @param path    原始路径字符串（仅用于错误日志）。
     * @param pos     下次解析的起始位置（跳过首段 key 及消化完的 [n] 和 .）。
     * @return 路径终点值，解析失败时返回无效 QVariant。
     */
    static QVariant walkNested(QVariant current, const QString& path, int pos)
    {
        int len = path.length();

        while (pos < len) {
            int dotPos = path.indexOf('.', pos);
            int bracketPos = path.indexOf('[', pos);
            int segEnd = len;

            if (dotPos >= 0 && bracketPos >= 0)
                segEnd = qMin(dotPos, bracketPos);
            else if (dotPos >= 0)
                segEnd = dotPos;
            else if (bracketPos >= 0)
                segEnd = bracketPos;

            QString key = path.mid(pos, segEnd - pos);
            if (key.isEmpty()) {
                Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                           QStringLiteral("empty key in path"));
                return QVariant();
            }

            // Type check: .key navigation requires QMetaType::QVariantMap as parent.
            if (current.userType() == QMetaType::QVariantMap) {
                QVariantMap map = current.toMap();
                if (!map.contains(key)) {
                    Diagnostics::reportRuntime(ErrorCode::NestedKeyMissing, path,
                                               QStringLiteral("%1 not found in object").arg(key));
                    return QVariant();
                }
                current = map.value(key);
            } else {
                Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                           QStringLiteral("cannot access %1 on non-object type %2")
                                               .arg(key, QLatin1String(current.typeName())));
                return QVariant();
            }

            pos = segEnd;
            pos = advanceBrackets(current, path, pos);
            if (pos < 0)
                return QVariant();

            if (pos < len) {
                if (path[pos] != '.') {
                    Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                               QStringLiteral("expected '.' or end, got %1")
                                                   .arg(path[pos]));
                    return QVariant();
                }
                pos++;
            }
        }

        return current;
    }

    /**
     * @brief 基于 QObject 属性的路径遍历。
     *
     * 解析首段 key，通过 QObject::property() 获取首段值，
     * 消化 [n] 索引后交给 walkNested 继续遍历。
     * 由 QPropertyContext 和 ItemPropertyContext 复用。
     *
     * @param obj  目标 QObject（不可为 nullptr）。
     * @param path 路径字符串，如 "device.cpu" 或 "items[0].name"。
     * @return 路径终点值，解析失败时返回无效 QVariant。
     */
    static QVariant walkPathFromObject(const QObject* obj, const QString& path)
    {
        if (!obj)
            return QVariant();

        int len = path.length();
        int dotPos = path.indexOf('.');
        int bracketPos = path.indexOf('[');
        int segEnd = len;

        if (dotPos >= 0 && bracketPos >= 0)
            segEnd = qMin(dotPos, bracketPos);
        else if (dotPos >= 0)
            segEnd = dotPos;
        else if (bracketPos >= 0)
            segEnd = bracketPos;

        QString firstKey = path.left(segEnd);
        QVariant current = obj->property(firstKey.toUtf8().constData());
        if (!current.isValid()) {
            Diagnostics::reportRuntime(ErrorCode::ObjectFirstKeyMissing, path,
                                       QStringLiteral("property %1 not found on object").arg(firstKey));
            return QVariant();
        }

        int pos = advanceBrackets(current, path, segEnd);
        if (pos < 0)
            return QVariant();

        if (pos < len) {
            if (path[pos] != '.') {
                Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                           QStringLiteral("expected '.' or end, got %1")
                                               .arg(path[pos]));
                return QVariant();
            }
            pos++;
        }

        return walkNested(current, path, pos);
    }

    /**
     * @brief 嵌套写入辅助：沿 path[pos:] 遍历 current 的嵌套结构，在叶子位置设置 value。
     *
     * 每步执行类型断言，失败时返回 false 且不修改 current（无副作用）。
     *
     * @param[in,out] current 当前值引用。成功时在叶子位置被修改。
     * @param path    原始路径（用于错误日志及遍历）。
     * @param pos     起始位置（跳过首段 key）。
     * @param value   待设置的值。
     * @return true 写入成功；false 类型/存在性校验失败。
     */
    static bool setWalkInto(QVariant& current, const QString& path, int pos, const QVariant& value)
    {
        int len = path.length();

        while (pos < len) {
            if (path[pos] == '[') {
                int closePos = path.indexOf(']', pos);
                if (closePos < 0) {
                    Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                               QStringLiteral("unmatched '['"));
                    return false;
                }
                QString indexStr = path.mid(pos + 1, closePos - pos - 1);
                bool ok;
                int index = indexStr.toInt(&ok);
                if (!ok || index < 0) {
                    Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                               QStringLiteral("invalid index %1").arg(indexStr));
                    return false;
                }
                if (current.userType() != QMetaType::QVariantList) {
                    Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                               QStringLiteral("cannot index into non-array type %1")
                                                   .arg(QLatin1String(current.typeName())));
                    return false;
                }
                QVariantList list = current.toList();
                if (index >= list.size()) {
                    Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                               QStringLiteral("index %1 out of bounds, size %2")
                                                   .arg(index).arg(list.size()));
                    return false;
                }

                pos = closePos + 1;
                if (pos >= len) {
                    list[index] = value;
                    current = list;
                    return true;
                }

                if (path[pos] != '.' && path[pos] != '[') {
                    Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                               QStringLiteral("expected '.' or '[' after ']', got %1")
                                                   .arg(path[pos]));
                    return false;
                }

                QVariant child = list.at(index);
                if (!setWalkInto(child, path, pos, value))
                    return false;
                list[index] = child;
                current = list;
                return true;

            } else if (path[pos] == '.') {
                pos++;

                int dotPos = path.indexOf('.', pos);
                int bracketPos = path.indexOf('[', pos);
                int segEnd = len;
                if (dotPos >= 0 && bracketPos >= 0)
                    segEnd = qMin(dotPos, bracketPos);
                else if (dotPos >= 0)
                    segEnd = dotPos;
                else if (bracketPos >= 0)
                    segEnd = bracketPos;

                QString key = path.mid(pos, segEnd - pos);
                if (key.isEmpty()) {
                    Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                               QStringLiteral("empty key in path"));
                    return false;
                }

                if (current.userType() != QMetaType::QVariantMap) {
                    Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                               QStringLiteral("cannot access %1 on non-object type %2")
                                                   .arg(key, QLatin1String(current.typeName())));
                    return false;
                }
                QVariantMap map = current.toMap();
                if (!map.contains(key)) {
                    Diagnostics::reportRuntime(ErrorCode::NestedKeyMissing, path,
                                               QStringLiteral("%1 not found in object").arg(key));
                    return false;
                }

                int savedSegEnd = segEnd;
                pos = savedSegEnd;
                if (pos >= len) {
                    map[key] = value;
                    current = map;
                    return true;
                }

                QVariant child = map.value(key);
                if (!setWalkInto(child, path, pos, value))
                    return false;
                map[key] = child;
                current = map;
                return true;

            } else {
                Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                           QStringLiteral("unexpected character %1")
                                               .arg(path[pos]));
                return false;
            }
        }

        return false;
    }

protected:
    /**
     * @brief 消化路径中的数组索引 [n]，修改 current 并返回新位置。
     *
     * @param[in,out] current 当前值，成功消化后指向索引的元素。
     * @param path            原始路径（用于错误日志）。
     * @param pos             当前处理位置。
     * @return 消化后的新位置，出错时返回 -1。
     */
    static int advanceBrackets(QVariant& current, const QString& path, int pos)
    {
        int len = path.length();

        while (pos < len && path[pos] == '[') {
            int closePos = path.indexOf(']', pos);
            if (closePos < 0) {
                Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                           QStringLiteral("unmatched '['"));
                return -1;
            }
            QString indexStr = path.mid(pos + 1, closePos - pos - 1);
            bool ok;
            int index = indexStr.toInt(&ok);
            if (!ok || index < 0) {
                Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                           QStringLiteral("invalid index %1").arg(indexStr));
                return -1;
            }
            // Type check: [n] indexing requires QMetaType::QVariantList as parent.
            if (current.userType() != QMetaType::QVariantList) {
                Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                           QStringLiteral("cannot index into non-array type %1")
                                               .arg(QLatin1String(current.typeName())));
                return -1;
            }
            QVariantList list = current.toList();
            if (index >= list.size()) {
                Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                           QStringLiteral("index %1 out of bounds, size %2")
                                               .arg(index).arg(list.size()));
                return -1;
            }
            current = list.at(index);
            pos = closePos + 1;
        }

        return pos;
    }

private:
    OnChanged m_onChanged;  ///< Registered callback for property change notifications.
};

// ========== PropertyProxy inline implementations ==========

inline PropertyProxy PropertyProxy::operator[](const QString& key) const
{
    return PropertyProxy(m_ctx, m_path + "." + key);
}

inline PropertyProxy PropertyProxy::operator[](int index) const
{
    return PropertyProxy(m_ctx, m_path + "[" + QString::number(index) + "]");
}

inline PropertyProxy::operator QVariant() const
{
    return m_ctx ? m_ctx->property(m_path) : QVariant();
}

inline PropertyProxy& PropertyProxy::operator=(const QVariant& value)
{
    if (m_ctx)
        m_ctx->setProperty(m_path, value);
    return *this;
}

} // namespace BroadItem
