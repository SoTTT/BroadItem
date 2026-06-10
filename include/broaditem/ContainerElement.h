#pragma once

#include "RenderableElement.h"

namespace BroadItem {

/// @brief 用装饰器（边距、边框、背景、内边距）包裹内容元素的容器。
class ContainerElement : public RenderableElement {
public:
    ContainerElement();

    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    /// @brief 设置此容器内的内容元素。
    void setContent(ElementPtr content);
    /// @brief 获取此容器内的内容元素。
    ElementPtr content() const { return m_content; }

    ElementPtr clone() const override;

    void resolveBindings(const LayoutContext& ctx) override;

private:
    ElementPtr m_content;  ///< The child content element wrapped by this container.
};

} // namespace BroadItem
