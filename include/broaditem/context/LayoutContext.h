#pragma once

#include <QVariant>
#include <QSizeF>
#include <broaditem/context/PropertyContext.h>
#include <broaditem/compat/Optional.h>

namespace BroadItem {

/// @brief 为布局流水线提供属性上下文访问。
class LayoutContext {
public:
    /// @brief 构造布局上下文（C++11 基线：带默认成员初始化的类不再是聚合，需显式构造函数）。
    /// @param c 属性上下文指针，可为空。
    explicit LayoutContext(PropertyContext* c = nullptr) : ctx(c) {}

    PropertyContext* ctx;  ///< 当前生效的属性上下文指针。

    /// @brief 检查上下文中是否存在指定属性。
    bool hasProperty(const QString& name) const
    {
        return ctx && ctx->hasProperty(name);
    }

    /// @brief 按名称读取属性值。
    QVariant property(const QString& name) const
    {
        return ctx ? ctx->property(name) : QVariant();
    }

    /// @brief 按名称设置属性值。
    void setProperty(const QString& name, const QVariant& value)
    {
        if (ctx)
            ctx->setProperty(name, value);
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
