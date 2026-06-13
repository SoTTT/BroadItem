#include <broaditem/text/TextElement.h>
#include <QPainter>
#include <QTextLayout>
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

/// @brief 返回 TextElement 支持的 XML 属性集合。
/// @return Reference to a static set including content, font, alignment, and box model attributes.
const QSet<QString>& TextElement::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{
        "width", "height",
        "content", "v-align", "h-align",
        "font-family", "font-size", "bold", "under-line",
        "wrap", "max-width", "color"
    } + boxModelAttributeNames();
    return attrs;
}

/// @brief 解析文本特定属性：content、b:content 绑定、字体、对齐、换行等。
/// @param xml The DOM element to parse.
void TextElement::parse(const QDomElement& xml)
{
    SizedElement::parse(xml);
    parseBoxModel(xml);
    validateAttributes(xml);

    m_text = xml.text().trimmed();

    bool hasContentLiteral = xml.hasAttribute("content");
    bool hasContentBinding = xml.hasAttribute(":content");

    if (hasContentLiteral && hasContentBinding) {
        qCritical() << "TextElement: 'content' and ':content' are mutually exclusive";
    }

    if (xml.hasAttribute("content")) {
        m_contentLiteral = xml.attribute("content");
        m_hasContentLiteral = true;
    }
    if (xml.hasAttribute(":content"))
        m_binding = Binding(":content", xml.attribute(":content"));
    if (xml.hasAttribute("v-align"))
        m_vAlign = xml.attribute("v-align");
    if (xml.hasAttribute("h-align"))
        m_hAlign = xml.attribute("h-align");
    if (xml.hasAttribute("font-family"))
        m_fontFamily = xml.attribute("font-family");
    if (xml.hasAttribute("font-size")) {
        if (validateDouble(xml.attribute("font-size"), "font-size", m_fontSize)) {
            if (m_fontSize <= 0) {
                qWarning() << "TextElement: font-size must be positive, got" << m_fontSize;
                m_fontSize = 12;
            }
        }
    }
    if (xml.hasAttribute("bold"))
        validateBool(xml.attribute("bold"), "bold", m_bold);
    if (xml.hasAttribute("under-line"))
        validateBool(xml.attribute("under-line"), "under-line", m_underLine);
    if (xml.hasAttribute("wrap"))
        validateBool(xml.attribute("wrap"), "wrap", m_wrap);
    if (xml.hasAttribute("max-width"))
        validateDouble(xml.attribute("max-width"), "max-width", m_maxWidth);
    if (xml.hasAttribute("color"))
        m_color = parseColor(xml.attribute("color"));
}

/// @brief 解析显示的文本：字面量内容、绑定的 b:content 属性或解析的 XML 文本。
/// @param ctx The layout context for property lookup.
/// @return The resolved text string.
QString TextElement::resolvedText(const LayoutContext& ctx) const
{
    if (m_hasContentLiteral)
        return m_contentLiteral;
    if (m_bindingsResolved)
        return m_text;
    if (m_binding.isValid() && ctx.hasProperty(m_binding.path())) {
        QVariant v = ctx.property(m_binding.path());
        if (v.userType() == QMetaType::QString)
            return v.toString();
        qCritical() << "TextElement: property" << m_binding.path()
                     << "expected QString, got" << v.typeName();
    }
    return m_text;
}

/// @brief 计算给定文本的渲染尺寸，考虑字体、换行和 max-width 约束。
/// @param text The text to measure.
/// @param constraints Available width/height constraints.
/// @return The computed text size.
QSizeF TextElement::computeTextSize(const QString& text, const LayoutConstraints& constraints) const
{
    QFont font = m_font;
    if (m_fontSize > 0)
        font.setPixelSize(static_cast<int>(m_fontSize));
    if (!m_fontFamily.isEmpty())
        font.setFamily(m_fontFamily);
    font.setBold(m_bold);
    font.setUnderline(m_underLine);

    QFontMetricsF fm(font);

    double maxW = m_maxWidth;
    if (constraints.availableWidth > 0 && (maxW < 0 || constraints.availableWidth < maxW))
        maxW = constraints.availableWidth;

    if (m_wrap && maxW > 0) {
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
            line.setLineWidth(maxW);
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

/// @brief 测量文本元素：解析文本、计算尺寸、应用显式宽度/高度。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @return The measured size including box model decoration.
MeasureResult TextElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    QString text = resolvedText(ctx);
    QSizeF sz = computeTextSize(text, constraints);
    double w = sz.width();
    double h = sz.height();

    if (hasWidth())
        w = width();
    if (hasHeight())
        h = height();

    w += boxModelWidth();
    h += boxModelHeight();

    return MeasureResult{QSizeF(w, h)};
}

/// @brief 存储分配的矩形并计算文本渲染的内容区域。
/// @param ctx The layout context (unused).
/// @param rect The bounding rectangle assigned to this text element.
void TextElement::layout(const LayoutContext& ctx, const QRectF& rect)
{
    Q_UNUSED(ctx)
    m_rect = rect;
    m_contentRect = contentRect(rect);
}

/// @brief 渲染文本元素：盒模型装饰，然后使用字体、对齐和换行渲染文本。
/// @param painter The QPainter to render onto.
/// @param ctx The layout context for property resolution.
void TextElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    renderBoxModel(painter, m_rect);

    QString text = resolvedText(ctx);
    if (text.isEmpty())
        return;

    QFont font = m_font;
    if (m_fontSize > 0)
        font.setPixelSize(static_cast<int>(m_fontSize));
    if (!m_fontFamily.isEmpty())
        font.setFamily(m_fontFamily);
    font.setBold(m_bold);
    font.setUnderline(m_underLine);

    painter->setFont(font);
    painter->setPen(m_color);

    const QRectF& textRect = m_contentRect;

    bool needClip = hasWidth() || hasHeight();
    if (needClip) {
        painter->save();
        painter->setClipRect(textRect);
    }

    QFontMetricsF fm(font);
    double maxW = m_maxWidth;
    if (textRect.width() > 0 && (maxW < 0 || textRect.width() < maxW))
        maxW = textRect.width();

    double x = textRect.x();
    double y = textRect.y();

    if (m_wrap && maxW > 0) {
        QTextLayout layout(text, font);
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        if (m_hAlign == "center")
            option.setAlignment(Qt::AlignHCenter);
        else if (m_hAlign == "right")
            option.setAlignment(Qt::AlignRight);
        else
            option.setAlignment(Qt::AlignLeft);
        layout.setTextOption(option);
        layout.beginLayout();
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(maxW);
        }
        layout.endLayout();

        double totalHeight = layout.boundingRect().height();
        double startY = y;
        if (m_vAlign == "center")
            startY = y + (textRect.height() - totalHeight) / 2.0;
        else if (m_vAlign == "bottom")
            startY = y + textRect.height() - totalHeight;

        for (int i = 0; i < layout.lineCount(); ++i) {
            QTextLine line = layout.lineAt(i);
            double lineX = x;
            if (m_hAlign == "center")
                lineX = x + (textRect.width() - line.naturalTextWidth()) / 2.0;
            else if (m_hAlign == "right")
                lineX = x + textRect.width() - line.naturalTextWidth();
            line.draw(painter, QPointF(lineX, startY + line.y()));
        }
    } else {
        QRectF bounding = fm.boundingRect(text);
        double textW = bounding.width();
        double textH = bounding.height();

        if (m_hAlign == "center")
            x = textRect.x() + (textRect.width() - textW) / 2.0;
        else if (m_hAlign == "right")
            x = textRect.x() + textRect.width() - textW;

        if (m_vAlign == "center")
            y = textRect.y() + (textRect.height() - textH) / 2.0;
        else if (m_vAlign == "bottom")
            y = textRect.y() + textRect.height() - textH;

        painter->drawText(QPointF(x, y + fm.ascent()), text);
    }

    if (needClip) {
        painter->restore();
    }
}

/// @brief 检查此文本元素是否通过 b:content 绑定指定属性。
/// @param name The property name to check.
/// @return True if the property matches the :content binding.
bool TextElement::bindsProperty(const QString& name) const
{
    return m_binding.bindsProperty(name);
}

/// @brief 创建此文本元素的深拷贝，包含所有属性。
/// @return A new TextElement with identical settings.
ElementPtr TextElement::clone() const
{
    auto copy = std::make_shared<TextElement>();
    copy->m_text = m_text;
    copy->m_contentLiteral = m_contentLiteral;
    copy->m_hasContentLiteral = m_hasContentLiteral;
    copy->m_bindingsResolved = false;  // Clones must resolve bindings fresh
    copy->m_binding = m_binding;
    copy->m_font = m_font;
    copy->m_vAlign = m_vAlign;
    copy->m_hAlign = m_hAlign;
    copy->m_bold = m_bold;
    copy->m_underLine = m_underLine;
    copy->m_wrap = m_wrap;
    copy->m_maxWidth = m_maxWidth;
    copy->m_fontSize = m_fontSize;
    copy->m_fontFamily = m_fontFamily;
    copy->m_color = m_color;
    copy->m_width = m_width;
    copy->m_height = m_height;
    copy->m_margin = m_margin;
    copy->m_border = m_border;
    copy->m_background = m_background;
    copy->m_padding = m_padding;
    copy->m_rect = m_rect;
    copy->m_contentRect = m_contentRect;
    return copy;
}

void TextElement::resolveBindings(const LayoutContext& ctx)
{
    if (m_binding.isValid()) {
        QVariant v = ctx.property(m_binding.path());
        if (v.isValid())
            m_text = v.toString();
        else
            m_text.clear();
        m_bindingsResolved = true;
    }
}

} // namespace BroadItem
