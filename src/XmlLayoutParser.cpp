#include "broaditem/XmlLayoutParser.h"
#include "broaditem/Element.h"
#include "broaditem/ContainerElement.h"
#include "broaditem/elements/TextElement.h"
#include "broaditem/elements/ColumnLayout.h"
#include "broaditem/elements/RowLayout.h"
#include "broaditem/elements/GridLayout.h"
#include "broaditem/elements/CellElement.h"
#include "broaditem/elements/ForElement.h"
#include "broaditem/elements/IfHasElement.h"
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
    if (!doc.setContent(xmlContent, &errorMsg, &errorLine, &errorColumn)) {
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
/// @param xml The DOM element to parse.
/// @return The parsed element, or nullptr on error.
ElementPtr XmlLayoutParser::parseNode(const QDomElement& xml)
{
    QString tag = xml.tagName();

    bool wrapFor = xml.hasAttribute(":of") && tag != "for" && tag != "if-has";
    bool wrapIfHas = xml.hasAttribute(":prop") && tag != "for" && tag != "if-has";

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
            bool hasControlChildren = false;
            for (const auto& child : grid->children()) {
                if (std::dynamic_pointer_cast<CellElement>(child)) {
                    cellCount++;
                } else if (std::dynamic_pointer_cast<ForElement>(child) ||
                           std::dynamic_pointer_cast<IfHasElement>(child)) {
                    hasControlChildren = true;
                } else {
                    qCritical() << "GridLayout: child must be <cell>, <for>, or <if-has>";
                    return nullptr;
                }
            }
            if (!hasControlChildren) {
                int expected = grid->columns() * grid->rows();
                if (cellCount != expected) {
                    qCritical() << "GridLayout: expected" << expected << "cells ("
                                << grid->columns() << "x" << grid->rows() << "), got" << cellCount;
                    return nullptr;
                }
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
        wrapper->setBindProperty(xml.attribute(":of"));
        wrapper->setTemplate(element);
        return wrapper;
    }

    if (wrapIfHas) {
        auto wrapper = std::make_shared<IfHasElement>();
        wrapper->setBindProperty(xml.attribute(":prop"));
        wrapper->setNot(false);
        wrapper->setChild(element);
        return wrapper;
    }

    return element;
}

} // namespace BroadItem
