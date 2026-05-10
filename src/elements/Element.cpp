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

} // namespace BroadItem
