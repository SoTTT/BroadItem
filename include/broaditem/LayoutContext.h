#pragma once

#include <QVariant>
#include <QStringList>
#include <QSizeF>
#include "PropertyContext.h"

namespace BroadItem {

/// @brief 为布局流水线提供属性上下文访问。
class LayoutContext {
public:
    PropertyContext* ctx = nullptr;  ///< Pointer to the active property context.

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
    double availableWidth = -1;
    double availableHeight = -1;
};

} // namespace BroadItem
