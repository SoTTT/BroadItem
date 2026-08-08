#include <broaditem/element/text/TextElement.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/compat/QtCompat.h>
#include <QPainter>
#include <QTextLayout>
#include <QDomElement>

namespace BroadItem {

namespace {

/// @brief HAlign 映射为 QTextOption 的水平对齐标志（换行路径使用）。
/// @param align 水平对齐枚举。
/// @return 对应的 Qt 对齐标志。
Qt::Alignment toQtAlignment(HAlign align)
{
    switch (align) {
    case HAlign::Center: return Qt::AlignHCenter;
    case HAlign::Right:  return Qt::AlignRight;
    case HAlign::Left:   break;
    }
    return Qt::AlignLeft;
}

} // namespace

/// @brief 返回 TextElement 支持的 XML 属性集合。
/// @return 静态集合引用，含 content、字体、对齐与盒模型属性。
const QSet<QString>& TextElement::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{
        "width", "height",
        "content", "v-align", "h-align",
        "font-family", "font-size", "bold", "underline",
        "wrap", "max-width", "color"
    } + boxModelAttributeNames();
    return attrs;
}

/// @brief 返回有求值路径的绑定属性集合：文本字体/颜色属性并入 SizedElement 集合。
/// @details content 走专用通道（不进通用绑定表）；v-align/h-align/wrap/max-width 为
///          布局策略属性，不参与绑定，其 b: 形式将被 parseBindings 拒绝（BI-P-023）。
/// @return 静态引用（沿用 supportedAttributes 的 static-union 写法）。
const QSet<QString>& TextElement::resolvedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{
        "color", "font-size", "bold", "underline", "font-family"
    } + SizedElement::resolvedAttributes();
    return attrs;
}

/// @brief 解析文本特定属性：content、b:content 绑定、字体、对齐、换行等。
/// @param xml 要解析的 DOM 元素。
void TextElement::parse(const QDomElement& xml)
{
    SizedElement::parse(xml);
    parseBoxModel(xml);
    validateAttributes(xml);

    m_text = xml.text().trimmed();

    bool hasContentLiteral = hasLiteralAttribute(xml, "content");
    bool hasContentBinding = xml.hasAttributeNS(BINDING_NS, "content");

    if (hasContentLiteral && hasContentBinding) {
        Diagnostics::reportParse(ErrorCode::MutexLiteralBinding,
                                 QStringLiteral("TextElement: 'content' and 'b:content' are mutually exclusive; "
                                                "the literal wins"));
    }

    if (hasLiteralAttribute(xml, "content")) {
        m_contentLiteral = literalAttribute(xml, "content");
        m_hasContentLiteral = true;
    }
    if (xml.hasAttributeNS(BINDING_NS, "content")) {
        auto attr = xml.attributes().namedItemNS(BINDING_NS, "content");
        m_binding = Binding(attr.nodeName(), xml.attributeNS(BINDING_NS, "content", QString()));
    }
    if (hasLiteralAttribute(xml, "v-align"))
        m_vAlign = parseVAlign(literalAttribute(xml, "v-align"), m_vAlign);
    if (hasLiteralAttribute(xml, "h-align"))
        m_hAlign = parseHAlign(literalAttribute(xml, "h-align"), m_hAlign);
    if (hasLiteralAttribute(xml, "font-family"))
        m_fontFamily = literalAttribute(xml, "font-family");
    if (hasLiteralAttribute(xml, "font-size")) {
        if (validateDouble(literalAttribute(xml, "font-size"), "font-size", m_fontSize)) {
            if (m_fontSize <= 0) {
                Diagnostics::reportParse(ErrorCode::LiteralOutOfRange,
                                         QStringLiteral("TextElement: font-size must be positive, got %1")
                                             .arg(m_fontSize));
                m_fontSize = kDefaultFontSize;
            }
        }
    }
    if (hasLiteralAttribute(xml, "bold"))
        validateBool(literalAttribute(xml, "bold"), "bold", m_bold);
    if (hasLiteralAttribute(xml, "underline"))
        validateBool(literalAttribute(xml, "underline"), "underline", m_underLine);
    if (hasLiteralAttribute(xml, "wrap"))
        validateBool(literalAttribute(xml, "wrap"), "wrap", m_wrap);
    if (hasLiteralAttribute(xml, "max-width")) {
        double val = -1;
        // 负值视为无限制（沿用哨兵时代 "maxW < 0 即无限制" 的语义）
        if (validateDouble(literalAttribute(xml, "max-width"), "max-width", val) && val >= 0)
            m_maxWidth = val;
    }
    if (hasLiteralAttribute(xml, "color"))
        m_color = parseColor(literalAttribute(xml, "color"));
}

/// @brief 按优先级解析文本：content 字面量 > b:content 绑定 > 标签文本。
///
/// 绑定值统一经 QVariant::toString() 转换（沿用旧 for 克隆路径的语义，
/// 黄金镜像基线即以此采集；旧静态路径的 "expected QString" qCritical
/// 随双模式删除一并移除）。
///
/// @param ctx 用于属性查找的布局上下文。
/// @return 解析后的文本字符串。
QString TextElement::resolveText(const LayoutContext& ctx) const
{
    if (m_hasContentLiteral)
        return m_contentLiteral;
    if (m_binding.isValid() && ctx.hasProperty(m_binding.path())) {
        QVariant v = ctx.property(m_binding.path());
        if (v.isValid())
            return v.toString();
    }
    return m_text;
}

/// @brief 物化：解析文本写入 TextNode。
/// @param ctx 布局上下文。
/// @return 新创建的 TextNode 实例节点。
std::unique_ptr<Node> TextElement::materialize(const LayoutContext& ctx) const
{
    auto node = makeUnique<TextNode>();
    node->element = this;
    node->text = resolveText(ctx);
    resolveStyle(ctx, node->style);
    resolveSize(ctx, node->style);
    node->color = resolveColor("color", ctx, m_color);
    node->fontSize = resolveDouble("font-size", ctx, m_fontSize);
    if (node->fontSize <= 0) {
        // 与 parse 行为对齐：非正字号回退为默认字号并报告诊断（运行时绑定值越界同 BI-R-011 族）。
        Diagnostics::reportRuntime(ErrorCode::BoundValueTypeError,
                                   bindingFor(QStringLiteral("font-size"))
                                       ? bindingFor(QStringLiteral("font-size"))->path() : QString(),
                                   QStringLiteral("TextElement: font-size must be positive, got %1")
                                       .arg(node->fontSize));
        node->fontSize = kDefaultFontSize;
    }
    node->bold = resolveBool("bold", ctx, m_bold);
    node->underLine = resolveBool("underline", ctx, m_underLine);
    node->fontFamily = resolveString("font-family", ctx, m_fontFamily);
    return node;
}

/// @brief 从 TextNode 快照重建 QFont：以 m_font 为基准，覆盖物化时求值的字体属性。
QFont TextElement::fontFromNode(const TextNode& node) const
{
    QFont font = m_font;
    if (node.fontSize > 0)
        font.setPixelSize(static_cast<int>(node.fontSize));
    if (!node.fontFamily.isEmpty())
        font.setFamily(node.fontFamily);
    font.setBold(node.bold);
    font.setUnderline(node.underLine);
    return font;
}

/// @brief 计算给定文本的渲染尺寸，考虑字体、换行和 max-width 约束。
/// @param text 要测量的文本。
/// @param constraints 可用宽高约束。
/// @param node 实例节点，读取物化时求值的字体属性（fontSize/fontFamily/bold/underLine）。
/// @return 计算出的文本尺寸。
QSizeF TextElement::computeTextSize(const QString& text, const LayoutConstraints& constraints, const TextNode& node) const
{
    QFont font = fontFromNode(node);
    QFontMetricsF fm(font);

    // 有效行宽：m_maxWidth 与可用宽度取较小者（两者皆可缺席）。
    Optional<double> maxW = m_maxWidth;
    if (constraints.availableWidth && (!maxW || *constraints.availableWidth < *maxW))
        maxW = constraints.availableWidth;

    if (m_wrap && maxW && *maxW > 0) {
        QTextLayout layout(text, font);
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(option);
        layout.beginLayout();
        double height = 0;
        double width = 0;
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(*maxW);
            width = std::max(width, line.naturalTextWidth());
            height += line.height();
        }
        layout.endLayout();
        return {width, height};
    } else {
        QRectF rect = fm.boundingRect(text);
        return {rect.width(), rect.height()};
    }
}

/// @brief 测量文本元素：读取节点文本、计算尺寸、应用显式宽度/高度。
/// @param ctx 布局上下文。
/// @param constraints 可用宽高约束。
/// @param node 实例节点（TextNode）。
/// @return 含盒模型装饰的测量尺寸。
MeasureResult TextElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    Q_UNUSED(ctx)
    const auto& textNode = static_cast<const TextNode&>(node);
    QSizeF sz = computeTextSize(textNode.text, constraints, textNode);
    double w = sz.width();
    double h = sz.height();

    if (node.style.width)
        w = *node.style.width;
    if (node.style.height)
        h = *node.style.height;

    w += boxModelWidth(node.style);
    h += boxModelHeight(node.style);

    return MeasureResult{QSizeF(w, h)};
}

/// @brief 存储分配的矩形；内容区域由 render 经 contentRect(node.rect, node.style) 现算
///        （与 ImageElement 同模式，不在节点缓存跨阶段中间值）。
/// @param ctx 布局上下文（未使用）。
/// @param rect 分配给此文本元素的矩形。
/// @param node 实例节点（TextNode）。
void TextElement::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    Q_UNUSED(ctx)
    node.rect = rect;
}

/// @brief 基线钩子：首行文字基线 = margin-top + border-width + padding-top + ascent。
///
/// 多行（wrap）文本统一取首行基线；字体从 TextNode 快照重建（同 render 路径）。
/// 供 RowLayout 的 cross-align="baseline" 分支使用。
///
/// @param ctx 布局上下文（未使用；字体属性已在物化时固化进节点）。
/// @param node 实例节点（TextNode）。
/// @return 矩形顶边到首行基线的距离（px）。
double TextElement::baselineOffset(const LayoutContext& ctx, const Node& node) const
{
    Q_UNUSED(ctx)
    const auto& textNode = static_cast<const TextNode&>(node);
    QFontMetricsF fm(fontFromNode(textNode));
    return node.style.margin.top + node.style.border.width + node.style.padding.top + fm.ascent();
}

/// @brief 渲染文本元素：盒模型装饰，然后使用字体、对齐和换行渲染节点文本。
/// @param painter 目标 QPainter。
/// @param ctx 布局上下文（未使用；文本已在物化时解析）。
/// @param node 实例节点（TextNode）。
void TextElement::render(QPainter* painter, const LayoutContext& ctx, const Node& node) const
{
    Q_UNUSED(ctx)
    renderBoxModel(painter, node.rect, node.style);

    const auto& textNode = static_cast<const TextNode&>(node);
    const QString& text = textNode.text;
    if (text.isEmpty())
        return;

    QFont font = fontFromNode(textNode);

    painter->setFont(font);
    painter->setPen(textNode.color);

    // 内容区域现算：contentRect 为纯函数，输入 node.rect/node.style 均在手边，
    // 无需经节点缓存传递（消除 layout→render 的隐式时序依赖）。
    const QRectF textRect = contentRect(node.rect, node.style);

    bool needClip = node.style.width.has_value() || node.style.height.has_value();
    if (needClip) {
        painter->save();
        painter->setClipRect(textRect);
    }

    QFontMetricsF fm(font);
    // 有效行宽：m_maxWidth 与内容区宽度取较小者（两者皆可缺席）。
    Optional<double> maxW = m_maxWidth;
    if (textRect.width() > 0 && (!maxW || textRect.width() < *maxW))
        maxW = textRect.width();

    double x = textRect.x();
    double y = textRect.y();

    if (m_wrap && maxW && *maxW > 0) {
        QTextLayout layout(text, font);
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        option.setAlignment(toQtAlignment(m_hAlign));
        layout.setTextOption(option);
        layout.beginLayout();
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(*maxW);
        }
        layout.endLayout();

        double totalHeight = layout.boundingRect().height();
        double startY = vAligned(y, textRect.height(), totalHeight, m_vAlign);

        for (int i = 0; i < layout.lineCount(); ++i) {
            QTextLine line = layout.lineAt(i);
            double lineX = hAligned(x, textRect.width(), line.naturalTextWidth(), m_hAlign);
            line.draw(painter, QPointF(lineX, startY + line.y()));
        }
    } else {
        QRectF bounding = fm.boundingRect(text);
        double textW = bounding.width();
        double textH = bounding.height();

        x = hAligned(textRect.x(), textRect.width(), textW, m_hAlign);
        y = vAligned(textRect.y(), textRect.height(), textH, m_vAlign);

        painter->drawText(QPointF(x, y + fm.ascent()), text);
    }

    if (needClip) {
        painter->restore();
    }
}

/// @brief 检查此文本元素是否通过通用绑定或 b:content 绑定指定属性。
/// @param name 要检查的属性名。
/// @return 属性命中通用绑定或 b:content 绑定时返回 true。
bool TextElement::bindsProperty(const QString& name) const
{
    return Element::bindsProperty(name) || m_binding.bindsProperty(name);
}

} // namespace BroadItem
