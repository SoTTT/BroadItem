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

} // namespace BroadItem
