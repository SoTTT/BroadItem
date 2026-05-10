#pragma once

#include "PropertyContext.h"
#include <QVariantMap>

namespace BroadItem {

/**
 * @brief 基于 QVariantMap 的属性上下文（默认实现）。
 *
 * 内部维护一个 QVariantMap 存储所有属性。支持扁平键和嵌套路径，
 * 路径遍历通过基类的共享引擎 walkNested 实现。
 *
 * 空值语义：setProperty(name, QVariant()) 从 map 中删除该键，
 * hasProperty() 返回 false。这使 `<if-has>` 的条件隐藏可以自然工作。
 *
 * 若构造 BroadItem 时未传入 PropertyContext，则自动创建本类实例。
 */
class MapPropertyContext : public PropertyContext {
public:
    /** @copydoc PropertyContext::property */
    QVariant property(const QString& name) const override
    {
        if (name.isEmpty())
            return QVariant();
        if (!name.contains('.') && !name.contains('['))
            return m_map.value(name);
        return resolveFirstThenWalk(name);
    }

    /** @copydoc PropertyContext::hasProperty */
    bool hasProperty(const QString& name) const override
    {
        if (name.isEmpty())
            return false;
        if (!name.contains('.') && !name.contains('['))
            return m_map.contains(name);
        return resolveFirstThenWalk(name).isValid();
    }

    /**
     * @brief 设置属性。null / Invalid 值表示移除。
     *
     * 新旧值相同时跳过通知（变更检测）。
     */
    void setProperty(const QString& name, const QVariant& value) override
    {
        if (!value.isValid() || value.isNull()) {
            if (m_map.remove(name) > 0)
                notifyChanged(name, QVariant());
            return;
        }
        auto it = m_map.find(name);
        if (it != m_map.end() && it.value() == value)
            return;
        m_map.insert(name, value);
        notifyChanged(name, value);
    }

private:
    QVariantMap m_map; ///< 属性存储。

    /**
     * @brief 解析路径首段，从 m_map 查找，再通过共享引擎遍历剩余部分。
     */
    QVariant resolveFirstThenWalk(const QString& path) const
    {
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
        if (!m_map.contains(firstKey)) {
            qCritical() << "MapPropertyContext:" << firstKey << "not found (path:" << path << ")";
            return QVariant();
        }

        QVariant current = m_map.value(firstKey);
        int pos = advanceBrackets(current, path, segEnd);
        if (pos < 0)
            return QVariant();

        if (pos < len) {
            if (path[pos] != '.') {
                qCritical() << "MapPropertyContext: expected '.' or end, got"
                            << path[pos] << "(path:" << path << ")";
                return QVariant();
            }
            pos++;
        }

        return walkNested(current, path, pos);
    }
};

} // namespace BroadItem
