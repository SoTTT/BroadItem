#pragma once

#include <broaditem/context/PropertyContext.h>
#include <broaditem/compat/QtCompat.h>
#include <QVariantMap>

namespace BroadItem {

/**
 * @brief 基于 QVariantMap 的属性上下文（默认实现）。
 *
 * 内部维护一个 QVariantMap 存储所有属性。支持扁平键和嵌套路径，
 * 路径遍历通过基类的共享引擎 walkSegments 实现。
 *
 * 空值语义：setProperty(name, QVariant()) 从 map 中删除该键，
 * hasProperty() 返回 false。这使 `<if>` 的条件隐藏可以自然工作。
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
        // 遍历失败的具体诊断已由 walkSegments 报告；
        // 首段未注入属正常状态（诊断标准静默清单），此处不再重复报告。
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
     * @brief 设置属性。null / Invalid 值表示移除（仅限扁平键，嵌套不删除）。
     *
     * 扁平键（不含 `.` 或 `[`）：保持现有行为——null 移除、同值跳过通知。
     * 嵌套路径（含 `.` 或 `[`）：进入嵌套写入模式，要求每段前缀存在且类型正确。
     *
     * 新旧值相同时跳过通知（变更检测）。
     */
    void setProperty(const QString& name, const QVariant& value) override
    {
        if (!name.contains('.') && !name.contains('[')) {
            // 扁平键——保持既有行为
            if (!value.isValid() || variantIsNull(value)) {
                if (m_map.remove(name) > 0)
                    notifyChanged(name, QVariant());
                return;
            }
            auto it = m_map.find(name);
            if (it != m_map.end() && it.value() == value)
                return;
            m_map.insert(name, value);
            notifyChanged(name, value);
            return;
        }
        // 嵌套路径
        setPropertyNested(name, value);
    }

private:
    QVariantMap m_map; ///< 属性存储。

    /**
     * @brief 嵌套路径写入。
     * 经 Expression 统一解析后取首段 key，从 m_map 取出 root，
     * 经由 setWalkSegments 修改后写回。写入成功后以首段 key 触发 notifyChanged。
     */
    void setPropertyNested(const QString& path, const QVariant& value)
    {
        const Expression expr(path);
        if (!expr.isValid()) {
            Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                       QStringLiteral("invalid path syntax"));
            return;
        }
        const auto& segs = expr.segments();
        // 语法保证首段必为键分段。
        const QString& firstKey = segs.front().key();
        if (!m_map.contains(firstKey)) {
            Diagnostics::reportRuntime(ErrorCode::NestedKeyMissing, path,
                                       QStringLiteral("%1 not found").arg(firstKey));
            return;
        }

        QVariant root = m_map.value(firstKey);
        if (!setWalkSegments(root, segs, 1, path, value))
            return;
        m_map[firstKey] = root;
        notifyChanged(firstKey, root);
    }

    /**
     * @brief 经 Expression 统一解析，从 m_map 查找首段，再遍历剩余分段。
     */
    QVariant resolveFirstThenWalk(const QString& path) const
    {
        const Expression expr(path);
        if (!expr.isValid()) {
            Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                       QStringLiteral("invalid path syntax"));
            return QVariant();
        }
        const auto& segs = expr.segments();
        const QString& firstKey = segs.front().key();
        if (!m_map.contains(firstKey))
            return QVariant();

        return walkSegments(m_map.value(firstKey), segs, 1, path);
    }
};

} // namespace BroadItem
