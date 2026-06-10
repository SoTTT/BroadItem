#pragma once

#include "Element.h"
#include <vector>

namespace BroadItem {

/// @brief 不直接参与渲染的控制元素的基类。
/// 控制元素（如 ForElement、IfHasElement）在运行时通过 expand() 展开为克隆实例。
/// 根据设计文档，控制元素没有装饰器。
class ControlElement : public Element {
public:
    virtual ~ControlElement() = default;

    /// @brief 控制元素的默认空操作测量实现。
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override {
        Q_UNUSED(ctx)
        Q_UNUSED(constraints)
        return MeasureResult{QSizeF(0, 0)};
    }
    /// @brief 控制元素的默认空操作布局实现。
    void layout(const LayoutContext& ctx, const QRectF& rect) override {
        Q_UNUSED(ctx)
        Q_UNUSED(rect)
    }
    /// @brief 控制元素的默认空操作渲染实现。
    void render(QPainter* painter, const LayoutContext& ctx) const override {
        Q_UNUSED(painter)
        Q_UNUSED(ctx)
    }

    /// @brief 展开控制元素为具体元素实例（默认返回空）。
    /// ForElement 返回每个迭代值的克隆实例；
    /// IfHasElement 返回条件渲染的克隆实例或空。
    virtual std::vector<ElementPtr> expand(const LayoutContext& ctx) const { return {}; }
};

} // namespace BroadItem
