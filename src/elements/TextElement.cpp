#include "broaditem/elements/TextElement.h"
#include <QPainter>
#include <QTextLayout>
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

const QSet<QString>& TextElement::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{
        "width", "height",
        "content", ":content", "v-align", "h-align",
        "font-family", "font-size", "bold", "under-line",
        "wrap", "max-width", "color"
    } + boxModelAttributeNames();
    return attrs;
}

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
        m_propertyName = xml.attribute(":content");
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

QString TextElement::resolvedText(const LayoutContext& ctx) const
{
    if (m_hasContentLiteral)
        return m_contentLiteral;
    if (!m_propertyName.isEmpty() && ctx.hasProperty(m_propertyName)) {
        QVariant v = ctx.property(m_propertyName);
        // Type check: :content binding requires QString.
        //   Pass — v.type() == QVariant::String; returns the bound text.
        //   Fail — v has a different type; qCritical logs the property
        //          name and actual type; falls through to default text.
        if (v.type() == QVariant::String)
            return v.toString();
        qCritical() << "TextElement: property" << m_propertyName
                     << "expected QString, got" << v.typeName();
    }
    return m_text;
}

QSizeF TextElement::computeTextSize(const QString& text, const LayoutConstraints& constraints) const
{
    QFont font = m_font;
    if (m_fontSize > 0)
        font.setPointSizeF(m_fontSize);
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

void TextElement::layout(const LayoutContext& ctx, const QRectF& rect)
{
    Q_UNUSED(ctx)
    m_rect = rect;
    m_contentRect = contentRect(rect);
}

void TextElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    renderBoxModel(painter, m_rect);

    QString text = resolvedText(ctx);
    if (text.isEmpty())
        return;

    QFont font = m_font;
    if (m_fontSize > 0)
        font.setPointSizeF(m_fontSize);
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

bool TextElement::bindsProperty(const QString& name) const
{
    return matchesProperty(m_propertyName, name);
}

ElementPtr TextElement::clone() const
{
    auto copy = std::make_shared<TextElement>();
    copy->m_text = m_text;
    copy->m_contentLiteral = m_contentLiteral;
    copy->m_hasContentLiteral = m_hasContentLiteral;
    copy->m_propertyName = m_propertyName;
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

void TextElement::interpolateValues(const QStringList& values)
{
    if (m_hasContentLiteral)
        m_contentLiteral = interpolate(m_contentLiteral, values);
    else
        m_text = interpolate(m_text, values);
}

} // namespace BroadItem
