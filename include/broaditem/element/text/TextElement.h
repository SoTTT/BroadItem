#pragma once

#include <broaditem/element/SizedElement.h>
#include <broaditem/expression/Binding.h>

namespace BroadItem {

/// @brief TextElement 的实例节点，持有物化时解析的文本与布局缓存。
struct TextNode : Node {
    QString text;        ///< 物化时解析后的文本。
    QRectF contentRect;  ///< 布局阶段缓存的内容区域。
    QColor color = Qt::black;   ///< 物化时求值的文字颜色。
    double fontSize = 12;       ///< 物化时求值的字体大小。
    bool bold = false;          ///< 物化时求值的加粗。
    bool underLine = false;     ///< 物化时求值的下划线。
    QString fontFamily;         ///< 物化时求值的字体族。
};

/// @brief 文本渲染元素，支持数据绑定、字体样式、对齐和自动换行。
class TextElement : public SizedElement {
public:
    void parse(const QDomElement& xml) override;
    std::unique_ptr<Node> materialize(const LayoutContext& ctx) const override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const override;
    void layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const override;
    void render(QPainter* painter, const LayoutContext& ctx, const Node& node) const override;
    bool bindsProperty(const QString& name) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return false; }

private:
    QString m_text;              ///< Static text content from the XML tag body.
    QString m_contentLiteral;    ///< Literal content from XML (before binding interpolation).
    bool m_hasContentLiteral = false; ///< Whether literal content was provided.
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

    /// @brief 按优先级解析文本：content 字面量 > b:content 绑定 > 标签文本。
    QString resolveText(const LayoutContext& ctx) const;
    /// @brief 计算给定约束下的渲染文本尺寸，字体取自 TextNode 快照。
    /// @param text The text to measure.
    /// @param constraints Available width/height constraints.
    /// @param node 实例节点，读取物化时求值的字体属性。
    /// @return The computed text size.
    QSizeF computeTextSize(const QString& text, const LayoutConstraints& constraints, const TextNode& node) const;
};

} // namespace BroadItem
