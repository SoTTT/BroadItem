#pragma once

#include <broaditem/context/LayoutContext.h>
#include <QRectF>
#include <QSizeF>
#include <QPainter>

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

/// @brief 协调元素树的三阶段流水线（测量、布局、渲染）。
class LayoutEngine {
public:
    /// @brief 在根元素上运行测量阶段，计算固有尺寸。
    static QSizeF measure(const ElementPtr& root, const LayoutContext& ctx, const LayoutConstraints& constraints);
    /// @brief 运行布局阶段，在给定的矩形内分配位置和尺寸。
    static void layout(const ElementPtr& root, const LayoutContext& ctx, const QRectF& rect);
    /// @brief 运行渲染阶段，将元素树绘制到给定的 painter 上。
    static void render(const ElementPtr& root, QPainter* painter, const LayoutContext& ctx);
};

} // namespace BroadItem
