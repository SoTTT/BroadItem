#include <broaditem/element/Element.h>
#include <QDomElement>

namespace BroadItem {

/// @brief 绑定属性的 XML 命名空间 URI。
const QString BINDING_NS = QStringLiteral("urn:broaditem:binding");

/// @brief 判断属性是否为 xmlns 命名空间声明。
/// @param attr The DOM attribute to check.
/// @return True if the attribute name starts with "xmlns".
bool isNamespaceDeclaration(const QDomAttr& attr)
{
    return attr.name().startsWith(QLatin1String("xmlns"));
}

/// @brief 判断属性是否属于绑定命名空间。
/// @param attr The DOM attribute to check.
/// @return True if attr.namespaceURI() == BINDING_NS.
bool isBindingAttribute(const QDomAttr& attr)
{
    return attr.namespaceURI() == BINDING_NS;
}

/// @brief 基础解析方法；子类覆盖以提取其属性。
/// @param xml The DOM element to parse.
void Element::parse(const QDomElement& xml)
{
    Q_UNUSED(xml)
}

/// @brief 安全地将字符串解析为 double。
/// @param value The string to parse.
/// @param defaultVal Value returned if parsing fails.
/// @return The parsed double, or defaultVal on failure.
double Element::parseDouble(const QString& value, double defaultVal)
{
    bool ok = false;
    double result = value.toDouble(&ok);
    return ok ? result : defaultVal;
}

/// @brief 将字符串解析为 QColor。
/// @param value The color string (any format QColor accepts).
/// @return The parsed QColor.
QColor Element::parseColor(const QString& value)
{
    return {value};
}

/// @brief 将字符串解析为布尔值。
/// @param value "true"/"1" returns true, everything else false.
/// @return The parsed boolean value.
bool Element::parseBool(const QString& value)
{
    return value.compare("true", Qt::CaseInsensitive) == 0 || value == "1";
}

/// @brief 默认实现：空操作。TextElement 等子类覆盖以从 context 解析绑定。
void Element::resolveBindings(const LayoutContext& ctx)
{
    Q_UNUSED(ctx)
}

/// @brief 检查属性名是否匹配绑定路径（支持点和括号子路径）。
/// @param bindPath The binding path (e.g. "user.name" or "items[0]").
/// @param propName The property name to match.
/// @return True if propName is a prefix match for bindPath.
bool Element::matchesProperty(const QString& bindPath, const QString& propName)
{
    if (bindPath == propName)
        return true;
    if (bindPath.startsWith(propName + ".") || bindPath.startsWith(propName + "["))
        return true;
    return false;
}

/// @brief 通过与 supportedAttributes() 比较来警告未知 XML 属性。
///
/// 遍历所有 XML 属性，跳过 xmlns 声明属性和绑定命名空间属性，
/// 其余属性若不在 supportedAttributes() 集合中则记录 qWarning。
///
/// @param xml The DOM element whose attributes to validate.
void Element::validateAttributes(const QDomElement& xml) const
{
    const QSet<QString>& known = supportedAttributes();
    QDomNamedNodeMap attrs = xml.attributes();
    for (int i = 0; i < attrs.size(); ++i) {
        QDomAttr attr = attrs.item(i).toAttr();
        if (isNamespaceDeclaration(attr))
            continue;
        if (isBindingAttribute(attr))
            continue;
        QString name = attr.name();
        if (!known.contains(name)) {
            qWarning() << xml.tagName() << ": unknown attribute" << name << "=\"" << attr.value() << "\"";
        }
    }
}

/// @brief 验证并将字符串解析为 double，失败时记录错误。
/// @param value The string to parse.
/// @param attrName Attribute name for error messages.
/// @param out Output parameter for the parsed value.
/// @return True if parsing succeeded.
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

/// @brief 验证并将字符串解析为整数，失败时记录错误。
/// @param value The string to parse.
/// @param attrName Attribute name for error messages.
/// @param out Output parameter for the parsed value.
/// @return True if parsing succeeded.
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

/// @brief 验证并将字符串解析为布尔值（"true"/"false"/"1"/"0"），失败时记录错误。
/// @param value The string to parse.
/// @param attrName Attribute name for error messages.
/// @param out Output parameter for the parsed value.
/// @return True if parsing succeeded.
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
