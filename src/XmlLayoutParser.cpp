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

    // Check there is only one child
    QDomElement secondChild = firstChild.nextSiblingElement();
    if (!secondChild.isNull()) {
        qWarning() << "<root> must have exactly one child element";
        return nullptr;
    }

    return parseNode(firstChild);
}

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
        // These are decorators and should be handled in parseNode
        return nullptr;
    }
    qWarning() << "Unknown element tag:" << tagName;
    return nullptr;
}

void XmlLayoutParser::applyPseudoAttributes(const ElementPtr& element, const QDomElement& xml)
{
    // Already handled in ContainerElement::parse via parseDecorators
    Q_UNUSED(element)
    Q_UNUSED(xml)
}

ElementPtr XmlLayoutParser::parseNode(const QDomElement& xml)
{
    QString tag = xml.tagName();

    // Handle decorators
    if (tag == "margin") {
        auto container = std::make_shared<ContainerElement>();
        container->parse(xml);
        QDomElement child = xml.firstChildElement();
        if (!child.isNull()) {
            auto content = parseNode(child);
            if (content) {
                // Merge margin into content's decorators
                content->decorators.margin = container->decorators.margin;
                return content;
            }
        }
        return container;
    }

    if (tag == "border") {
        auto container = std::make_shared<ContainerElement>();
        container->parse(xml);
        QDomElement child = xml.firstChildElement();
        if (!child.isNull()) {
            auto content = parseNode(child);
            if (content) {
                content->decorators.border = container->decorators.border;
                return content;
            }
        }
        return container;
    }

    if (tag == "background") {
        auto container = std::make_shared<ContainerElement>();
        container->parse(xml);
        QDomElement child = xml.firstChildElement();
        if (!child.isNull()) {
            auto content = parseNode(child);
            if (content) {
                content->decorators.background = container->decorators.background;
                return content;
            }
        }
        return container;
    }

    if (tag == "padding") {
        auto container = std::make_shared<ContainerElement>();
        container->parse(xml);
        QDomElement child = xml.firstChildElement();
        if (!child.isNull()) {
            auto content = parseNode(child);
            if (content) {
                content->decorators.padding = container->decorators.padding;
                return content;
            }
        }
        return container;
    }

    // Handle pseudo-attributes: for, if-has
    QString forBind = xml.attribute("for");
    QString ifHasBind = xml.attribute("if-has");
    bool hasFor = xml.hasAttribute("for");
    bool hasIfHas = xml.hasAttribute("if-has");

    ElementPtr element = createElement(tag);
    if (!element)
        return nullptr;

    element->parse(xml);

    // Parse children for container elements
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
        // Leaf container (e.g., text with no child but has decorators)
        // TextElement handles its own text content, no children to parse
    }

    // Wrap with for/if-has if pseudo-attributes exist
    if (hasFor) {
        auto wrapper = std::make_shared<ForElement>();
        wrapper->setBindProperty(forBind);
        wrapper->setTemplate(element);
        return wrapper;
    }

    if (hasIfHas) {
        auto wrapper = std::make_shared<IfHasElement>();
        wrapper->setBindProperty(ifHasBind);
        wrapper->setNot(false);
        wrapper->setChild(element);
        return wrapper;
    }

    return element;
}

} // namespace BroadItem
