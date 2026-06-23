#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/element/Element.h>
#include <broaditem/element/ContainerElement.h>
#include <broaditem/element/text/TextElement.h>
#include <broaditem/element/layout/ColumnLayout.h>
#include <broaditem/element/layout/RowLayout.h>
#include <broaditem/element/layout/GridLayout.h>
#include <broaditem/element/layout/CellElement.h>
#include <broaditem/element/control/ForElement.h>
#include <broaditem/element/control/IfHasElement.h>
#include <QDomDocument>
#include <QFile>
#include <QDebug>

namespace BroadItem {

/// @brief 读取并将 XML 布局文件解析为元素树。
/// @param filePath Absolute or relative path to the XML file.
/// @return The root element, or nullptr on failure.
ElementPtr XmlLayoutParser::parseFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open XML file:" << filePath;
        return nullptr;
    }
    QString content = QString::fromUtf8(file.readAll());
    return parseString(content);
}

/// @brief 将 XML 字符串解析为元素树。
/// @param xmlContent The XML markup to parse.
/// @return The root element, or nullptr on parse failure.
ElementPtr XmlLayoutParser::parseString(const QString& xmlContent)
{
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    /// Qt 命名空间处理会剥离纯空白文本节点（如用于对齐的缩进），
    /// 这是预期行为，BroadItem 布局引擎不依赖这些空白。
    if (!doc.setContent(xmlContent, true, &errorMsg, &errorLine, &errorColumn)) {
        qWarning() << "XML parse error at line" << errorLine << "column" << errorColumn << ":" << errorMsg;
        return nullptr;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "root") {
        qWarning() << "Root element must be <root>";
        return nullptr;
    }

    QDomElement firstChild;
    for (QDomElement child = root.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
        firstChild = child;
        break;
    }

    if (firstChild.isNull()) {
        qWarning() << "<root> must have exactly one child element";
        return nullptr;
    }

    QDomElement secondChild = firstChild.nextSiblingElement();
    if (!secondChild.isNull()) {
        qWarning() << "<root> must have exactly one child element";
        return nullptr;
    }

    return parseNode(firstChild);
}

/// @brief 工厂方法：根据标签名创建元素实例。
/// @param tagName The XML tag name (e.g. "text", "column", "row").
/// @return A new element of the corresponding type, or nullptr for unknown tags.
ElementPtr XmlLayoutParser::createElement(const QString& tagName)
{
    if (tagName == "text")
        return std::make_shared<TextElement>();
    if (tagName == "column")
        return std::make_shared<ColumnLayout>();
    if (tagName == "row")
        return std::make_shared<RowLayout>();
    if (tagName == "grid")
        return std::make_shared<GridLayout>();
    if (tagName == "cell")
        return std::make_shared<CellElement>();
    if (tagName == "for")
        return std::make_shared<ForElement>();
    if (tagName == "if-has")
        return std::make_shared<IfHasElement>();
    if (tagName == "margin" || tagName == "border" || tagName == "padding" || tagName == "background") {
        qWarning() << "Deprecated decorator tag <" << tagName << ">. Use inline attributes instead (e.g. margin=\"4\" border-width=\"1\").";
        return nullptr;
    }
    qWarning() << "Unknown element tag:" << tagName;
    return nullptr;
}

/// @brief 递归地将 DOM 元素及其子元素解析为 BroadItem 元素树。
/// 使用 hasAttributeNS/attributeNS 检测 b:of/b:prop/b:as 等绑定属性。
/// @param xml The DOM element to parse.
/// @return The parsed element, or nullptr on error.
ElementPtr XmlLayoutParser::parseNode(const QDomElement& xml)
{
    QString tag = xml.tagName();

    bool wrapFor = xml.hasAttributeNS(BINDING_NS, "of") && tag != "for" && tag != "if-has";
    bool wrapIfHas = xml.hasAttributeNS(BINDING_NS, "prop") && tag != "for" && tag != "if-has";

    ElementPtr element = createElement(tag);
    if (!element)
        return nullptr;

    element->parse(xml);

    if (!element->canHaveChildren() && !xml.firstChildElement().isNull()) {
        qCritical() << "<" << tag << "> should not have child elements";
        return nullptr;
    }

    auto container = std::dynamic_pointer_cast<ContainerElement>(element);
    auto column = std::dynamic_pointer_cast<ColumnLayout>(element);
    auto row = std::dynamic_pointer_cast<RowLayout>(element);
    auto grid = std::dynamic_pointer_cast<GridLayout>(element);
    auto cell = std::dynamic_pointer_cast<CellElement>(element);
    auto forEl = std::dynamic_pointer_cast<ForElement>(element);
    auto ifEl = std::dynamic_pointer_cast<IfHasElement>(element);

    if (column || row || grid) {
        for (QDomElement child = xml.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
            auto childEl = parseNode(child);
            if (childEl) {
                if (column) column->addChild(childEl);
                else if (row) row->addChild(childEl);
                else if (grid) grid->addChild(childEl);
            }
        }
        if (grid) {
            int cellCount = 0;
            for (const auto& child : grid->children()) {
                if (std::dynamic_pointer_cast<CellElement>(child)) {
                    cellCount++;
                } else {
                    qCritical() << "GridLayout: child must be <cell>";
                    return nullptr;
                }
            }
            int expected = grid->columns() * grid->rows();
            if (cellCount != expected) {
                qCritical() << "GridLayout: expected" << expected << "cells ("
                            << grid->columns() << "x" << grid->rows() << "), got" << cellCount;
                return nullptr;
            }
        }
    } else if (cell) {
        QDomElement child = xml.firstChildElement();
        if (!child.isNull()) {
            auto childEl = parseNode(child);
            if (childEl)
                cell->setContent(childEl);
        }
    } else if (forEl) {
        QDomElement child = xml.firstChildElement();
        if (!child.isNull()) {
            auto templ = parseNode(child);
            if (templ)
                forEl->setTemplate(templ);
        }
        if (xml.hasAttributeNS(BINDING_NS, "as"))
            forEl->setAsVariable(xml.attributeNS(BINDING_NS, "as", QString()));
    } else if (ifEl) {
        QDomElement child = xml.firstChildElement();
        if (!child.isNull()) {
            auto childEl = parseNode(child);
            if (childEl)
                ifEl->setChild(childEl);
        }
    } else if (container && xml.firstChildElement().isNull()) {
        // Leaf container
    }

    if (wrapFor) {
        auto wrapper = std::make_shared<ForElement>();
        wrapper->setBindProperty(xml.attributeNS(BINDING_NS, "of", QString()));
        if (xml.hasAttributeNS(BINDING_NS, "as"))
            wrapper->setAsVariable(xml.attributeNS(BINDING_NS, "as", QString()));
        wrapper->setTemplate(element);
        return wrapper;
    }

    if (wrapIfHas) {
        auto wrapper = std::make_shared<IfHasElement>();
        wrapper->setBindProperty(xml.attributeNS(BINDING_NS, "prop", QString()));
        wrapper->setNot(false);
        wrapper->setChild(element);
        return wrapper;
    }

    if (tag != "for" && xml.hasAttributeNS(BINDING_NS, "as") && !xml.hasAttributeNS(BINDING_NS, "of"))
        qWarning() << "<" << tag << "> has b:as but no b:of; b:as only works with iteration";

    return element;
}

} // namespace BroadItem
