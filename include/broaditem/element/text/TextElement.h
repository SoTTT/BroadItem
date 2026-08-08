#pragma once

#include <broaditem/element/SizedElement.h>
#include <broaditem/element/Alignment.h>
#include <broaditem/compat/Optional.h>
#include <broaditem/expression/Binding.h>

namespace BroadItem {

/// @brief TextElement 的实例节点，持有物化时解析的文本与样式快照。
struct TextNode : Node {
    QString text;        ///< 物化时解析后的文本。
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
    double baselineOffset(const LayoutContext& ctx, const Node& node) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return false; }

protected:
    /// @brief 返回有求值路径的绑定属性集合：文本字体/颜色属性 + 尺寸 + 盒模型。
    /// @return 静态引用（materialize 逐项求值 color/font-size/bold/underline/font-family）。
    const QSet<QString>& resolvedAttributes() const override;

private:
    static constexpr double kDefaultFontSize = 12;  ///< 默认字号（px）：模板成员缺省与越界回退共用。

    QString m_text;              ///< XML 标签体内的静态文本内容。
    QString m_contentLiteral;    ///< XML 中的字面量 content（绑定插值前）。
    bool m_hasContentLiteral = false; ///< 是否提供了字面量 content。
    Binding m_binding{"b:content", QString{}}; ///< b:content 属性的绑定。
    QFont m_font;                ///< 渲染所用的字体。
    VAlign m_vAlign = VAlign::Top;   ///< 垂直对齐。
    HAlign m_hAlign = HAlign::Left;  ///< 水平对齐。
    bool m_bold = false;         ///< 是否加粗。
    bool m_underLine = false;    ///< 是否下划线。
    bool m_wrap = false;         ///< 是否在元素宽度处自动换行。
    Optional<double> m_maxWidth;   ///< 换行的最大宽度（空 = 无限制）。
    double m_fontSize = kDefaultFontSize;  ///< 字号（px）。
    QString m_fontFamily;        ///< 字体族名。
    QColor m_color = Qt::black;  ///< 文字颜色。

    /// @brief 按优先级解析文本：content 字面量 > b:content 绑定 > 标签文本。
    QString resolveText(const LayoutContext& ctx) const;
    /// @brief 计算给定约束下的渲染文本尺寸，字体取自 TextNode 快照。
    /// @param text 要测量的文本。
    /// @param constraints 可用宽高约束。
    /// @param node 实例节点，读取物化时求值的字体属性。
    /// @return 计算出的文本尺寸。
    QSizeF computeTextSize(const QString& text, const LayoutConstraints& constraints, const TextNode& node) const;
    /// @brief 从 TextNode 快照重建 QFont（fontSize/fontFamily/bold/underLine 覆盖 m_font 基准）。
    /// @param node 实例节点，读取物化时求值的字体属性。
    /// @return 重建后的字体；供 measure/render/baselineOffset 共用，避免多处漂移。
    QFont fontFromNode(const TextNode& node) const;
};

} // namespace BroadItem
