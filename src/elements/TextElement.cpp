#include "broaditem/elements/TextElement.h"
#include <QPainter>
#include <QTextLayout>
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

void TextElement::parse(const QDomElement& xml)
{
    SizedElement::parse(xml);

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
        m_fontSize = parseDouble(xml.attribute("font-size"), 12);
        if (m_fontSize <= 0) {
            qWarning() << "TextElement: font-size must be positive, got" << m_fontSize;
            m_fontSize = 12;
        }
    }
    if (xml.hasAttribute("bold"))
        m_bold = parseBool(xml.attribute("bold"));
    if (xml.hasAttribute("under-line"))
        m_underLine = parseBool(xml.attribute("under-line"));
    if (xml.hasAttribute("wrap"))
        m_wrap = parseBool(xml.attribute("wrap"));
    if (xml.hasAttribute("max-width"))
        m_maxWidth = parseDouble(xml.attribute("max-width"), -1);
    if (xml.hasAttribute("color"))
        m_color = parseColor(xml.attribute("color"));
}

QString TextElement::resolvedText(const LayoutContext& ctx) const
{
    if (m_hasContentLiteral)
        return m_contentLiteral;
    if (!m_propertyName.isEmpty() && ctx.hasProperty(m_propertyName)) {
        QVariant v = ctx.property(m_propertyName);
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
    Size result;
    result.width = sz.width();
    result.height = sz.height();

    // Apply specified size as requested size (if any)
    if (hasWidth())
        result.width = width();
    if (hasHeight())
        result.height = height();

    return MeasureResult{result};
}

void TextElement::layout(const LayoutContext& ctx, const Rect& rect)
{
    Q_UNUSED(ctx)
    m_rect = rect;
}

void TextElement::render(QPainter* painter, const LayoutContext& ctx) const
{
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

    // Clip if size is specified (content may exceed allocated rect)
    bool needClip = hasWidth() || hasHeight();
    if (needClip) {
        painter->save();
        painter->setClipRect(m_rect.toQRectF());
    }

    QFontMetricsF fm(font);
    double maxW = m_maxWidth;
    if (m_rect.size.width > 0 && (maxW < 0 || m_rect.size.width < maxW))
        maxW = m_rect.size.width;

    double x = m_rect.pos.x;
    double y = m_rect.pos.y;

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
            startY = y + (m_rect.size.height - totalHeight) / 2.0;
        else if (m_vAlign == "bottom")
            startY = y + m_rect.size.height - totalHeight;

        for (int i = 0; i < layout.lineCount(); ++i) {
            QTextLine line = layout.lineAt(i);
            double lineX = x;
            if (m_hAlign == "center")
                lineX = x + (m_rect.size.width - line.naturalTextWidth()) / 2.0;
            else if (m_hAlign == "right")
                lineX = x + m_rect.size.width - line.naturalTextWidth();
            line.draw(painter, QPointF(lineX, startY + line.y()));
        }
    } else {
        QRectF bounding = fm.boundingRect(text);
        double textW = bounding.width();
        double textH = bounding.height();

        if (m_hAlign == "center")
            x = m_rect.pos.x + (m_rect.size.width - textW) / 2.0;
        else if (m_hAlign == "right")
            x = m_rect.pos.x + m_rect.size.width - textW;

        if (m_vAlign == "center")
            y = m_rect.pos.y + (m_rect.size.height - textH) / 2.0;
        else if (m_vAlign == "bottom")
            y = m_rect.pos.y + m_rect.size.height - textH;

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
    copy->decorators = decorators;
    copy->m_rect = m_rect;
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
