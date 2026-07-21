#include <broaditem/element/layout/ColumnLayout.h>
#include <QPainter>
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

/// @brief 返回 ColumnLayout 支持的 XML 属性集合。
/// @return Reference to a static set of attribute names.
const QSet<QString>& ColumnLayout::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{"main-align", "cross-align", "space", "main-stretch"} + boxModelAttributeNames();
    return attrs;
}

/// @brief 解析列布局配置的 XML 属性。
/// @param xml The DOM element to parse.
void ColumnLayout::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    validateAttributes(xml);
    if (hasLiteralAttribute(xml, "main-align"))
        m_mainAlign = literalAttribute(xml, "main-align");
    if (hasLiteralAttribute(xml, "cross-align"))
        m_crossAlign = literalAttribute(xml, "cross-align");
    if (hasLiteralAttribute(xml, "space"))
        validateDouble(literalAttribute(xml, "space"), "space", m_space);
    if (hasLiteralAttribute(xml, "main-stretch")) {
        const QString value = literalAttribute(xml, "main-stretch").toLower();
        if (value == "true" || value == "false") {
            m_mainStretch = (value == "true");
        } else {
            qWarning() << "ColumnLayout: 'main-stretch' must be 'true' or 'false', got" << literalAttribute(xml, "main-stretch") << "- defaulting to false";
            m_mainStretch = false;
        }
    }
}

/// @brief 测量列：累加子节点高度，跟踪最大子节点宽度。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @param node 实例节点（children 已物化）。
/// @return The measured size including box model decoration.
MeasureResult ColumnLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    double totalHeight = 0;
    double maxWidth = 0;

    LayoutConstraints childConstraints = constraints;
    double decoH = boxModelHeight(node.style);
    double decoW = boxModelWidth(node.style);

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    const auto& children = node.children;

    if (m_mainStretch && !children.empty()) {
        double maxChildHeight = 0;
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            maxChildHeight = std::max(maxChildHeight, result.intrinsicSize.height());
            maxWidth = std::max(maxWidth, result.intrinsicSize.width());
        }
        totalHeight = maxChildHeight * static_cast<double>(children.size())
                      + m_space * static_cast<double>(children.size() - 1);
    } else {
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            totalHeight += result.intrinsicSize.height();
            maxWidth = std::max(maxWidth, result.intrinsicSize.width());
            if (i + 1 < children.size())
                totalHeight += m_space;
        }
    }

    QSizeF sz(maxWidth + decoW, totalHeight + decoH);
    return MeasureResult{sz};
}

/// @brief 在给定矩形内垂直布局子节点，应用对齐和间距。
/// @param ctx The layout context.
/// @param rect The bounding rectangle assigned to this column.
/// @param node 实例节点，rect 写入 node.rect。
void ColumnLayout::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    node.rect = rect;

    QRectF cr = contentRect(rect, node.style);

    layoutChildren(ctx, cr, node);
}

/// @brief 在内容区域内定位子节点，处理主轴和交叉轴对齐。
/// @param ctx The layout context.
/// @param contentRect The content area rect (excluding box model decoration).
/// @param node 实例节点。
void ColumnLayout::layoutChildren(const LayoutContext& ctx, const QRectF& contentRect, Node& node) const
{
    auto& children = node.children;
    if (children.empty())
        return;

    std::vector<QSizeF> childSizes;
    double totalIntrinsicHeight = 0;
    LayoutConstraints childConstraints;
    childConstraints.availableWidth = contentRect.width();
    childConstraints.availableHeight = -1;

    if (m_mainStretch) {
        double maxChildHeight = 0;
        std::vector<bool> hasExplicitHeight;
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            childSizes.push_back(result.intrinsicSize);
            maxChildHeight = std::max(maxChildHeight, result.intrinsicSize.height());
            hasExplicitHeight.push_back(children[i]->style.height >= 0);
        }
        childConstraints.availableHeight = maxChildHeight;
        childSizes.clear();
        for (size_t i = 0; i < children.size(); ++i) {
            if (!hasExplicitHeight[i]) {
                LayoutConstraints stretchedConstraints = childConstraints;
                stretchedConstraints.availableHeight = maxChildHeight;
                auto result = children[i]->element->measure(ctx, stretchedConstraints, *children[i]);
                childSizes.push_back(QSizeF(result.intrinsicSize.width(), maxChildHeight));
            } else {
                auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
                childSizes.push_back(result.intrinsicSize);
            }
        }
        totalIntrinsicHeight = maxChildHeight * static_cast<double>(children.size())
                               + m_space * static_cast<double>(children.size() - 1);
    } else {
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            childSizes.push_back(result.intrinsicSize);
            totalIntrinsicHeight += result.intrinsicSize.height();
            if (i + 1 < children.size())
                totalIntrinsicHeight += m_space;
        }
    }

    double extraSpace = contentRect.height() - totalIntrinsicHeight;
    double offsetY = contentRect.y();

    double spacing = m_space;
    if (m_mainAlign == "center") {
        offsetY += extraSpace / 2.0;
    } else if (m_mainAlign == "end") {
        offsetY += extraSpace;
    } else if (m_mainAlign == "space-between" && children.size() > 1) {
        spacing = m_space + extraSpace / (static_cast<double>(children.size()) - 1);
    } else if (m_mainAlign == "space-around" && !children.empty()) {
        double perItem = extraSpace / static_cast<double>(children.size());
        offsetY += perItem / 2.0;
        spacing = m_space + perItem;
    } else if (m_mainAlign == "space-evenly" && !children.empty()) {
        double perGap = extraSpace / static_cast<double>(children.size() + 1);
        offsetY += perGap;
        spacing = m_space + perGap;
    }

    for (size_t i = 0; i < children.size(); ++i) {
        double childHeight = childSizes[i].height();
        double childWidth = childSizes[i].width();

        if (m_crossAlign == "stretch") {
            if (children[i]->style.width < 0)
                childWidth = contentRect.width();
        }

        double childX = contentRect.x();
        if (m_crossAlign == "center") {
            childX = contentRect.x() + (contentRect.width() - childWidth) / 2.0;
        } else if (m_crossAlign == "end") {
            childX = contentRect.x() + contentRect.width() - childWidth;
        }

        QRectF childRect(childX, offsetY, childWidth, childHeight);
        children[i]->element->layout(ctx, childRect, *children[i]);
        offsetY += childHeight + spacing;
    }
}

} // namespace BroadItem
