#pragma once

#include "Element.h"
#include "BoxModel.h"

namespace BroadItem {

/// @brief 具有可视化表现和盒模型装饰器的元素基类。
class RenderableElement : public Element {
public:
    virtual ~RenderableElement() = default;

    Margin  m_margin;       ///< Margin around the element.
    Border  m_border;       ///< Border around the element.
    Background m_background; ///< Background fill of the element.
    Padding m_padding;      ///< Padding inside the element.

    /// @brief 从 XML 解析盒模型属性（边距、边框、背景、内边距）。
    void parseBoxModel(const QDomElement& xml);
    /// @brief 渲染盒模型装饰器（边距、边框、背景、内边距）。
    void renderBoxModel(QPainter* painter, const QRectF& rect) const;
    /// @brief 从外部矩形减去盒模型装饰器，计算内容区域矩形。
    QRectF contentRect(const QRectF& outerRect) const;

    /// @brief 返回水平盒模型总宽度（边距+边框+内边距）。
    double boxModelWidth() const {
        return m_margin.width() + m_border.width * 2 + m_padding.width();
    }
    /// @brief 返回垂直盒模型总高度（边距+边框+内边距）。
    double boxModelHeight() const {
        return m_margin.height() + m_border.width * 2 + m_padding.height();
    }

    /// @brief 返回支持的盒模型属性名称集合。
    static const QSet<QString>& boxModelAttributeNames();
};

} // namespace BroadItem
