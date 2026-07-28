#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/element/Element.h>
#include <broaditem/element/ContainerElement.h>
#include <broaditem/element/text/TextElement.h>
#include <broaditem/element/image/ImageElement.h>
#include <broaditem/element/layout/ColumnLayout.h>
#include <broaditem/element/layout/RowLayout.h>
#include <broaditem/element/layout/GridLayout.h>
#include <broaditem/element/layout/CellElement.h>
#include <broaditem/element/control/ForElement.h>
#include <broaditem/element/control/IfElement.h>
#include <QDomDocument>
#include <QFile>

namespace BroadItem {

namespace {

/// @brief 元素路径段 RAII 守卫：进入压栈、离开弹栈。
struct SegmentGuard {
    explicit SegmentGuard(const QString& segment) { Diagnostics::pushElementSegment(segment); }
    ~SegmentGuard() { Diagnostics::popElementSegment(); }
};

/// @brief 计算元素的元素路径段：无同名兄弟为 "tag"，否则为 "tag[i]"（1 基序号）。
/// @param xml 目标 DOM 元素。
/// @return 路径段。
QString segmentFor(const QDomElement& xml)
{
    const QString tag = xml.tagName();
    int index = 1;
    bool multiple = false;
    for (QDomElement s = xml.previousSiblingElement(tag); !s.isNull(); s = s.previousSiblingElement(tag)) {
        ++index;
        multiple = true;
    }
    if (!multiple && xml.nextSiblingElement(tag).isNull())
        return tag;
    return QStringLiteral("%1[%2]").arg(tag).arg(index);
}

} // namespace

/// @brief 读取并将 XML 布局文件解析为元素树。
/// @param filePath  XML 文件路径。
/// @param collector 本次调用专用诊断收集器（可空）。
/// @return 根元素；文件无法打开或发生 Abort 级错误返回 nullptr。
ElementPtr XmlLayoutParser::parseFile(const QString& filePath, ErrorCollector* collector)
{
    // 会话提前建立：文件打不开的 BI-P-001 同样经本次收集器投递
    Diagnostics::ParseSession session(filePath, collector);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        Diagnostics::reportParse(ErrorCode::FileOpenFailed,
                                 QStringLiteral("cannot open XML file: %1").arg(filePath));
        return nullptr;
    }
    QString content = QString::fromUtf8(file.readAll());
    return parseStringInternal(content, filePath, collector);
}

/// @brief 将 XML 字符串解析为元素树。
/// @param xmlContent XML 文本。
/// @param collector  本次调用专用诊断收集器（可空）。
/// @return 根元素；发生 Abort 级错误返回 nullptr。
ElementPtr XmlLayoutParser::parseString(const QString& xmlContent, ErrorCollector* collector)
{
    return parseStringInternal(xmlContent, QString(), collector);
}

/// @brief 在解析会话下解析 XML 字符串：全收集语义——树遍历不提前中止，
/// 末尾按 sawAbort 决定返回契约（nullptr ⇔ 存在 Abort 级错误）。
///
/// root 结构级错误（BI-P-002/003/004）无法继续遍历，记录后 fail-fast。
///
/// @param xmlContent XML 文本。
/// @param file       布局文件路径（直接解析字符串时为空）。
/// @param collector  本次调用专用收集器（可空）。
/// @return 根元素；发生 Abort 级错误返回 nullptr。
ElementPtr XmlLayoutParser::parseStringInternal(const QString& xmlContent, const QString& file,
                                                ErrorCollector* collector)
{
    Diagnostics::ParseSession session(file, collector);

    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    /// Qt 命名空间处理会剥离纯空白文本节点（如用于对齐的缩进），
    /// 这是预期行为，BroadItem 布局引擎不依赖这些空白。
    if (!doc.setContent(xmlContent, true, &errorMsg, &errorLine, &errorColumn)) {
        Diagnostics::reportParse(ErrorCode::XmlSyntaxError, errorMsg, QString(), errorLine, errorColumn);
        return nullptr;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != QLatin1String("root")) {
        Diagnostics::reportParse(ErrorCode::RootNotRoot, QStringLiteral("root element must be <root>"));
        return nullptr;
    }

    QDomElement firstChild;
    for (QDomElement child = root.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
        firstChild = child;
        break;
    }

    if (firstChild.isNull()) {
        Diagnostics::reportParse(ErrorCode::RootChildCount,
                                 QStringLiteral("<root> must have exactly one child element"));
        return nullptr;
    }

    if (!firstChild.nextSiblingElement().isNull()) {
        Diagnostics::reportParse(ErrorCode::RootChildCount,
                                 QStringLiteral("<root> must have exactly one child element"));
        return nullptr;
    }

    const SegmentGuard rootGuard(QStringLiteral("root"));
    ElementPtr tree = parseNode(firstChild);
    return session.sawAbort() ? nullptr : tree;
}

/// @brief 工厂方法：根据标签名创建元素实例。
///
/// 未知标签报 BI-P-005（Error/Abort）；deprecated 装饰器标签报 BI-P-006 并忽略。
///
/// @param tagName The XML tag name (e.g. "text", "column", "row").
/// @return A new element of the corresponding type, or nullptr for unknown tags.
ElementPtr XmlLayoutParser::createElement(const QString& tagName)
{
    if (tagName == QLatin1String("text"))
        return std::make_shared<TextElement>();
    if (tagName == QLatin1String("image"))
        return std::make_shared<ImageElement>();
    if (tagName == QLatin1String("column"))
        return std::make_shared<ColumnLayout>();
    if (tagName == QLatin1String("row"))
        return std::make_shared<RowLayout>();
    if (tagName == QLatin1String("grid"))
        return std::make_shared<GridLayout>();
    if (tagName == QLatin1String("cell"))
        return std::make_shared<CellElement>();
    if (tagName == QLatin1String("for"))
        return std::make_shared<ForElement>();
    if (tagName == QLatin1String("if"))
        return std::make_shared<IfElement>();
    if (tagName == QLatin1String("margin") || tagName == QLatin1String("border")
        || tagName == QLatin1String("padding") || tagName == QLatin1String("background")) {
        Diagnostics::reportParse(ErrorCode::DeprecatedDecoratorTag,
                                 QStringLiteral("deprecated decorator tag <%1>; use inline attributes instead "
                                                "(e.g. margin=\"4\" border-width=\"1\")").arg(tagName));
        return nullptr;
    }
    Diagnostics::reportParse(ErrorCode::UnknownElementTag,
                             QStringLiteral("unknown element tag: %1").arg(tagName));
    return nullptr;
}

/// @brief 递归地将 DOM 元素及其子元素解析为 BroadItem 元素树。
///
/// 返回 nullptr 仅表示该子树构造失败；全收集语义下父循环继续处理兄弟节点，
/// 最终是否整体失败由会话 sawAbort 决定（见 parseStringInternal）。
///
/// @param xml The DOM element to parse.
/// @return The parsed element, or nullptr on error.
ElementPtr XmlLayoutParser::parseNode(const QDomElement& xml)
{
    const SegmentGuard guard(segmentFor(xml));
    QString tag = xml.tagName();

    bool wrapFor = xml.hasAttributeNS(BINDING_NS, "of") && tag != QLatin1String("for") && tag != QLatin1String("if");
    bool wrapIf = xml.hasAttributeNS(BINDING_NS, "prop") && tag != QLatin1String("for") && tag != QLatin1String("if");

    ElementPtr element = createElement(tag);
    if (!element)
        return nullptr;

    element->parse(xml);
    // parseBindings 对控制元素同样调用；保留名 {of,prop,as,content,not} 在 parseBindings 内部豁免，
    // 上方与下方的伪属性包装逻辑（wrapFor/wrapIf）不受影响
    element->parseBindings(xml);

    if (!element->canHaveChildren() && !xml.firstChildElement().isNull()) {
        Diagnostics::reportParse(ErrorCode::LeafWithChildren,
                                 QStringLiteral("<%1> should not have child elements").arg(tag));
        return nullptr;
    }

    auto container = std::dynamic_pointer_cast<ContainerElement>(element);
    auto column = std::dynamic_pointer_cast<ColumnLayout>(element);
    auto row = std::dynamic_pointer_cast<RowLayout>(element);
    auto grid = std::dynamic_pointer_cast<GridLayout>(element);
    auto cell = std::dynamic_pointer_cast<CellElement>(element);
    auto forEl = std::dynamic_pointer_cast<ForElement>(element);
    auto ifEl = std::dynamic_pointer_cast<IfElement>(element);

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
                    Diagnostics::reportParse(ErrorCode::GridNonCellChild,
                                             QStringLiteral("GridLayout: child must be <cell>"));
                    return nullptr;
                }
            }
            int expected = grid->columns() * grid->rows();
            if (cellCount != expected) {
                Diagnostics::reportParse(ErrorCode::GridCellCountMismatch,
                                         QStringLiteral("GridLayout: expected %1 cells (%2x%3), got %4")
                                             .arg(expected).arg(grid->columns()).arg(grid->rows()).arg(cellCount));
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
        if (!ifEl->isConditionValid()) {
            Diagnostics::reportParse(ErrorCode::IfMissingProp,
                                     QStringLiteral("<if> requires b:prop to specify the condition path"));
            return nullptr;
        }
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

    if (wrapIf) {
        auto wrapper = std::make_shared<IfElement>();
        wrapper->setBindProperty(xml.attributeNS(BINDING_NS, "prop", QString()));
        wrapper->setNot(false);
        wrapper->setChild(element);
        return wrapper;
    }

    if (tag != QLatin1String("for") && xml.hasAttributeNS(BINDING_NS, "as") && !xml.hasAttributeNS(BINDING_NS, "of"))
        Diagnostics::reportParse(ErrorCode::AsWithoutOf,
                                 QStringLiteral("<%1> has b:as but no b:of; b:as only works with iteration").arg(tag));

    return element;
}

} // namespace BroadItem
