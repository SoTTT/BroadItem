#pragma once

#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QString>
#include <QObject>
#include <QDebug>
#include <functional>

namespace BroadItem {

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
     * 每步执行类型断言，失败时通过 qCritical 报告错误并返回无效值。
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
                qCritical() << "PropertyContext: empty key in path" << path;
                return QVariant();
            }

            // Type check: .key navigation requires QVariant::Map as parent.
            //   Pass — current.type() == QVariant::Map; extracts the nested
            //          value by key.  Missing key is a separate error (qCritical
            //          + returns invalid QVariant).
            //   Fail — current is not a Map; qCritical logs the key name and
            //          actual type; returns QVariant() to terminate the path.
            if (current.type() == QVariant::Map) {
                QVariantMap map = current.toMap();
                if (!map.contains(key)) {
                    qCritical() << "PropertyContext:" << key
                                << "not found in object (path:" << path << ")";
                    return QVariant();
                }
                current = map.value(key);
            } else {
                qCritical() << "PropertyContext: cannot access" << key
                            << "on non-object type" << current.typeName()
                            << "(path:" << path << ")";
                return QVariant();
            }

            pos = segEnd;
            pos = advanceBrackets(current, path, pos);
            if (pos < 0)
                return QVariant();

            if (pos < len) {
                if (path[pos] != '.') {
                    qCritical() << "PropertyContext: expected '.' or end, got"
                                << path[pos] << "(path:" << path << ")";
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
            qCritical() << "PropertyContext: property" << firstKey
                        << "not found on object (path:" << path << ")";
            return QVariant();
        }

        int pos = advanceBrackets(current, path, segEnd);
        if (pos < 0)
            return QVariant();

        if (pos < len) {
            if (path[pos] != '.') {
                qCritical() << "PropertyContext: expected '.' or end, got"
                            << path[pos] << "(path:" << path << ")";
                return QVariant();
            }
            pos++;
        }

        return walkNested(current, path, pos);
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
                qCritical() << "PropertyContext: unmatched '[' (path:" << path << ")";
                return -1;
            }
            QString indexStr = path.mid(pos + 1, closePos - pos - 1);
            bool ok;
            int index = indexStr.toInt(&ok);
            if (!ok || index < 0) {
                qCritical() << "PropertyContext: invalid index" << indexStr
                            << "(path:" << path << ")";
                return -1;
            }
            // Type check: [n] indexing requires QVariant::List as parent.
            //   Pass — current.type() == QVariant::List; reads list[n].
            //          Out-of-bounds is a separate error (qCritical + returns -1).
            //   Fail — current is not a List; qCritical logs the actual type
            //          name; returns -1 to terminate the path.
            if (current.type() != QVariant::List) {
                qCritical() << "PropertyContext: cannot index into non-array type"
                            << current.typeName() << "(path:" << path << ")";
                return -1;
            }
            QVariantList list = current.toList();
            if (index >= list.size()) {
                qCritical() << "PropertyContext: index" << index
                            << "out of bounds, size" << list.size()
                            << "(path:" << path << ")";
                return -1;
            }
            current = list.at(index);
            pos = closePos + 1;
        }

        return pos;
    }

private:
    OnChanged m_onChanged;
};

} // namespace BroadItem
