#pragma once

#include <broaditem/element/RenderableElement.h>

namespace BroadItem {

/// @brief 用装饰器（边距、边框、背景、内边距）包裹内容元素的容器。
///
/// 物化时 content 展开为 node->children（控制元素透明展开）。
/// content 物化出多个节点时（如 \<cell\> 的 content 带 b:of）按垂直堆叠处理：
/// measure 累加高度取最大宽度，layout 依次分配，render 依次渲染。
class ContainerElement : public RenderableElement {
public:
    ContainerElement();

    void parse(const QDomElement& xml) override;
    std::unique_ptr<Node> materialize(const LayoutContext& ctx) const override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const override;
    void layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const override;
    void render(QPainter* painter, const LayoutContext& ctx, const Node& node) const override;
    bool bindsProperty(const QString& name) const override;

    /// @brief 设置此容器内的内容元素。
    void setContent(ElementPtr content);
    /// @brief 获取此容器内的内容元素。
    ElementPtr content() const { return m_content; }

private:
    ElementPtr m_content;  ///< The child content element wrapped by this container.
};

} // namespace BroadItem
