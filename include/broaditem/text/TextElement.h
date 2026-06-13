#pragma once

#include <broaditem/element/SizedElement.h>
#include <broaditem/expression/Binding.h>

namespace BroadItem {

/// @brief 文本渲染元素，支持数据绑定、字体样式、对齐和自动换行。
class TextElement : public SizedElement {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    ElementPtr clone() const override;
    void resolveBindings(const LayoutContext& ctx) override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return false; }

private:
    QString m_text;              ///< Static text content.
    QString m_contentLiteral;    ///< Literal content from XML (before binding interpolation).
    bool m_hasContentLiteral = false; ///< Whether literal content was provided.
    bool m_bindingsResolved = false; ///< Set after resolveBindings() to skip live lookup.
    Binding m_binding{"b:content", QString{}}; ///< Binding for the b:content attribute.
    QFont m_font;                ///< Font used for rendering.
    QString m_vAlign = "baseline"; ///< Vertical alignment ("baseline", "top", "center", "bottom").
    QString m_hAlign = "left";   ///< Horizontal alignment ("left", "center", "right").
    bool m_bold = false;         ///< Whether text is bold.
    bool m_underLine = false;    ///< Whether text is underlined.
    bool m_wrap = false;         ///< Whether text wraps at the element width.
    double m_maxWidth = -1;      ///< Maximum width for text wrapping (-1 = no limit).
    double m_fontSize = 12;      ///< Font size in points.
    QString m_fontFamily;        ///< Font family name.
    QColor m_color = Qt::black;  ///< Text color.
    QRectF m_contentRect;        ///< Cached content rectangle after layout.

    /// @brief 解析有效的文本内容，如果配置了数据绑定则应用。
    QString resolvedText(const LayoutContext& ctx) const;
    /// @brief 计算给定约束下的渲染文本尺寸。
    QSizeF computeTextSize(const QString& text, const LayoutConstraints& constraints) const;
};

} // namespace BroadItem
