#pragma once

#include <QVariant>
#include <QSizeF>
#include <broaditem/context/PropertyContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/compat/Optional.h>

namespace BroadItem {

/// @brief 为布局流水线提供属性上下文访问。
class LayoutContext {
public:
    /**
     * @brief `<for>` 迭代项作用域节点。
     *
     * 在 ForElement::materializeChildren() 栈上构造，经 parent 串成链以支持
     * 嵌套 for：内层 as 变量优先命中，外层 as 变量沿 parent 链保持可见。
     * 节点与其 item 上下文的生命周期均限于一次 materializeChildren() 调用内。
     */
    struct Scope {
        const MapPropertyContext* item;  ///< 当前迭代项的属性上下文（只读，可为空）。
        QString asVariable;              ///< `<for as="...">` 迭代变量名，用于前缀剥离。
        const Scope* parent;             ///< 外层 for 的作用域节点（无外层时为空）。
    };

    /// @brief 构造布局上下文（C++11 基线：带默认成员初始化的类不再是聚合，需显式构造函数）。
    /// @param c 属性上下文指针，可为空。
    explicit LayoutContext(PropertyContext* c = nullptr) : ctx(c) {}

    PropertyContext* ctx;            ///< 当前生效的全局属性上下文指针。
    const Scope* scope = nullptr;    ///< `<for>` 作用域链头（无迭代作用域时为空）。

    /// @brief 检查上下文中是否存在指定属性。
    ///
    /// 沿 scope 链由内向外逐节点查找（与旧链式项级上下文的三级查找语义等价：
    /// as 前缀剥离查迭代项 → 无前缀精确查迭代项 → 回退外层/全局），链穷尽后落全局 ctx。
    bool hasProperty(const QString& name) const
    {
        QVariant ignored;
        if (lookupInScopes(name, &ignored))
            return true;
        return ctx && ctx->hasProperty(name);
    }

    /// @brief 按名称读取属性值（查找顺序同 hasProperty）。
    QVariant property(const QString& name) const
    {
        QVariant value;
        if (lookupInScopes(name, &value))
            return value;
        return ctx ? ctx->property(name) : QVariant();
    }

    /// @brief 按名称设置属性值。
    ///
    /// 写操作穿透作用域链、始终直写全局 ctx（迭代项上下文只读）。
    void setProperty(const QString& name, const QVariant& value)
    {
        if (ctx)
            ctx->setProperty(name, value);
    }

private:
    /**
     * @brief 沿 scope 链由内向外查找属性。
     *
     * 每个节点：name 以该节点 "asVariable." 开头则剥前缀后在其 item 中查；
     * 否则在其 item 中精确查（扁平键或点号路径，嵌套遍历由
     * MapPropertyContext 的 resolveFirstThenWalk/walkSegments 完成）。
     * 命中即返回 true 并经 out 带出值；链穷尽返回 false。
     */
    static bool lookupInScopeChain(const QString& name, QVariant* out, const Scope* chainHead)
    {
        for (const Scope* node = chainHead; node; node = node->parent) {
            if (!node->item)
                continue;
            const QString prefix = node->asVariable + QStringLiteral(".");
            if (name.startsWith(prefix)) {
                QVariant v = node->item->property(name.mid(prefix.length()));
                if (v.isValid()) {
                    if (out) *out = v;
                    return true;
                }
            } else {
                QVariant v = node->item->property(name);
                if (v.isValid()) {
                    if (out) *out = v;
                    return true;
                }
            }
        }
        return false;
    }

    /// @brief 实例壳：以本上下文的 scope 链头调用公共查找实现。
    bool lookupInScopes(const QString& name, QVariant* out) const
    {
        return lookupInScopeChain(name, out, scope);
    }
};

/// @brief 测量阶段的结果，包含计算出的固有尺寸。
struct MeasureResult {
    QSizeF intrinsicSize;
};

/// @brief 传递给测量阶段的约束，限制可用空间。
struct LayoutConstraints {
    /// @brief 构造约束（C++11 基线：带默认成员初始化的类不再是聚合，需显式构造函数）。
    /// @param w 可用宽度，空表示内容驱动。
    /// @param h 可用高度，空表示内容驱动。
    LayoutConstraints(const Optional<double>& w = nullopt, const Optional<double>& h = nullopt)
        : availableWidth(w), availableHeight(h) {}

    Optional<double> availableWidth;   ///< 可用宽度（空 = 内容驱动）。
    Optional<double> availableHeight;  ///< 可用高度（空 = 内容驱动）。
};

} // namespace BroadItem
