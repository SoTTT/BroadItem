#include "broaditem/Element.h"
#include <QDomElement>

namespace BroadItem {

void Element::parse(const QDomElement& xml)
{
    Q_UNUSED(xml)
}

double Element::parseDouble(const QString& value, double defaultVal)
{
    bool ok = false;
    double result = value.toDouble(&ok);
    return ok ? result : defaultVal;
}

QColor Element::parseColor(const QString& value)
{
    return {value};
}

bool Element::parseBool(const QString& value)
{
    return value.compare("true", Qt::CaseInsensitive) == 0 || value == "1";
}

QString Element::interpolate(const QString& text, const QStringList& values)
{
    QString result = text;
    int index = 0;
    int pos = 0;
    while ((pos = result.indexOf("{}", pos)) != -1 && index < values.size()) {
        result.replace(pos, 2, values[index]);
        pos += values[index].length();
        ++index;
    }
    return result;
}

void Element::interpolateValues(const QStringList& values)
{
    Q_UNUSED(values)
}

bool Element::matchesProperty(const QString& bindPath, const QString& propName)
{
    if (bindPath == propName)
        return true;
    if (bindPath.startsWith(propName + ".") || bindPath.startsWith(propName + "["))
        return true;
    return false;
}

void Element::validateAttributes(const QDomElement& xml) const
{
    const QSet<QString>& known = supportedAttributes();
    QDomNamedNodeMap attrs = xml.attributes();
    for (int i = 0; i < attrs.size(); ++i) {
        QDomAttr attr = attrs.item(i).toAttr();
        QString name = attr.name();
        if (!known.contains(name)) {
            qWarning() << xml.tagName() << ": unknown attribute" << name << "=\"" << attr.value() << "\"";
        }
    }
}

bool Element::validateDouble(const QString& value, const QString& attrName, double& out)
{
    bool ok = false;
    out = value.toDouble(&ok);
    if (!ok) {
        qCritical() << "Attribute" << attrName << "expects a number, got \"" << value << "\"";
        return false;
    }
    return true;
}

bool Element::validateInt(const QString& value, const QString& attrName, int& out)
{
    bool ok = false;
    out = value.toInt(&ok);
    if (!ok) {
        qCritical() << "Attribute" << attrName << "expects an integer, got \"" << value << "\"";
        return false;
    }
    return true;
}

bool Element::validateBool(const QString& value, const QString& attrName, bool& out)
{
    QString v = value.toLower().trimmed();
    if (v == "true" || v == "1") {
        out = true;
        return true;
    }
    if (v == "false" || v == "0") {
        out = false;
        return true;
    }
    qCritical() << "Attribute" << attrName << "expects true/false or 1/0, got \"" << value << "\"";
    return false;
}

const QSet<QString>& Element::decoratorAttributeNames()
{
    static const QSet<QString> attrs = {
        "margin", "margin-left", "margin-right", "margin-top", "margin-bottom",
        "padding", "padding-left", "padding-right", "padding-top", "padding-bottom",
        "border-radius", "border-style", "border-width", "border-color",
        "background-color", "background-radius", "background-transparent"
    };
    return attrs;
}

void Element::parseDecorators(const QDomElement& xml)
{
    if (isControlElement)
        return;
    // Margin pseudo-properties
    if (xml.hasAttribute("margin"))
        decorators.margin.left = decorators.margin.right = decorators.margin.top = decorators.margin.bottom = parseDouble(xml.attribute("margin"));
    if (xml.hasAttribute("margin-left"))
        decorators.margin.left = parseDouble(xml.attribute("margin-left"));
    if (xml.hasAttribute("margin-right"))
        decorators.margin.right = parseDouble(xml.attribute("margin-right"));
    if (xml.hasAttribute("margin-top"))
        decorators.margin.top = parseDouble(xml.attribute("margin-top"));
    if (xml.hasAttribute("margin-bottom"))
        decorators.margin.bottom = parseDouble(xml.attribute("margin-bottom"));

    // Padding pseudo-properties
    if (xml.hasAttribute("padding"))
        decorators.padding.left = decorators.padding.right = decorators.padding.top = decorators.padding.bottom = parseDouble(xml.attribute("padding"));
    if (xml.hasAttribute("padding-left"))
        decorators.padding.left = parseDouble(xml.attribute("padding-left"));
    if (xml.hasAttribute("padding-right"))
        decorators.padding.right = parseDouble(xml.attribute("padding-right"));
    if (xml.hasAttribute("padding-top"))
        decorators.padding.top = parseDouble(xml.attribute("padding-top"));
    if (xml.hasAttribute("padding-bottom"))
        decorators.padding.bottom = parseDouble(xml.attribute("padding-bottom"));

    // Border pseudo-properties
    if (xml.hasAttribute("border-radius"))
        decorators.border.radius = parseDouble(xml.attribute("border-radius"));
    if (xml.hasAttribute("border-style"))
        decorators.border.style = xml.attribute("border-style");
    if (xml.hasAttribute("border-width"))
        decorators.border.width = parseDouble(xml.attribute("border-width"));
    if (xml.hasAttribute("border-color"))
        decorators.border.color = parseColor(xml.attribute("border-color"));

    // Background pseudo-properties
    if (xml.hasAttribute("background-color")) {
        decorators.background.color = parseColor(xml.attribute("background-color"));
        decorators.background.enabled = true;
    }
    if (xml.hasAttribute("background-radius")) {
        decorators.background.radius = parseDouble(xml.attribute("background-radius"));
        decorators.background.enabled = true;
    }
    if (xml.hasAttribute("background-transparent")) {
        decorators.background.transparent = parseDouble(xml.attribute("background-transparent"));
        decorators.background.enabled = true;
    }
}

void Element::renderDecorators(QPainter* painter, const Rect& rect) const
{
    if (isControlElement)
        return;
    double mLeft = decorators.margin.left;
    double mTop = decorators.margin.top;
    double mRight = decorators.margin.right;
    double mBottom = decorators.margin.bottom;

    Rect borderRect;
    borderRect.pos.x = rect.pos.x + mLeft;
    borderRect.pos.y = rect.pos.y + mTop;
    borderRect.size.width = std::max(0.0, rect.size.width - mLeft - mRight);
    borderRect.size.height = std::max(0.0, rect.size.height - mTop - mBottom);

    // Background fills border-box area
    Rect bgRect = borderRect;

    // Render background
    if (decorators.background.visible()) {
        QColor c = decorators.background.color;
        c.setAlphaF(1.0 - decorators.background.transparent);
        painter->setBrush(c);
        painter->setPen(Qt::NoPen);

        double radius = decorators.background.radius;
        if (radius > 0)
            painter->drawRoundedRect(bgRect.toQRectF(), radius, radius);
        else
            painter->drawRect(bgRect.toQRectF());
    }

    // Render border
    if (decorators.border.visible()) {
        QPen pen(decorators.border.color);
        pen.setWidthF(decorators.border.width);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        double radius = decorators.border.radius;
        double halfW = decorators.border.width / 2.0;
        QRectF adjusted(borderRect.pos.x + halfW, borderRect.pos.y + halfW,
                        std::max(0.0, borderRect.size.width - decorators.border.width),
                        std::max(0.0, borderRect.size.height - decorators.border.width));

        if (radius > 0)
            painter->drawRoundedRect(adjusted, radius, radius);
        else
            painter->drawRect(adjusted);
    }
}

Rect Element::contentRect(const Rect& outerRect) const
{
    Rect content;
    content.pos.x = outerRect.pos.x + decorators.margin.left + decorators.border.width + decorators.padding.left;
    content.pos.y = outerRect.pos.y + decorators.margin.top + decorators.border.width + decorators.padding.top;
    content.size.width = std::max(0.0, outerRect.size.width - decorators.totalWidth());
    content.size.height = std::max(0.0, outerRect.size.height - decorators.totalHeight());
    return content;
}

} // namespace BroadItem
