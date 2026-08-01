#include <broaditem/element/layout/RowLayout.h>
#include <algorithm>
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 RowLayout 支持的 XML 属性集合。
/// @return Reference to a static set of attribute names.
const QSet<QString>& RowLayout::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{"main-align", "cross-align", "space", "main-stretch"} + boxModelAttributeNames();
    return attrs;
}

/// @brief 解析行布局配置的 XML 属性。
/// @param xml The DOM element to parse.
void RowLayout::parse(const QDomElement& xml)
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
        if (!validateBool(literalAttribute(xml, "main-stretch"), "main-stretch", m_mainStretch))
            m_mainStretch = false;
    }
}

/// @brief 测量行：累加子节点宽度，跟踪最大子节点高度。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @param node 实例节点（children 已物化）。
/// @return The measured size including box model decoration.
MeasureResult RowLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    double totalWidth = 0;
    double maxHeight = 0;

    LayoutConstraints childConstraints = constraints;
    double decoW = boxModelWidth(node.style);
    double decoH = boxModelHeight(node.style);

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    const auto& children = node.children;

    // baseline 交叉轴模式：行内容高 = 最大基线 + 最大基线以下高度
    const bool baselineMode = (m_crossAlign == "baseline");
    double maxBaseline = 0;
    double maxBelowBaseline = 0;

    if (m_mainStretch && !children.empty()) {
        // 主轴等距拉伸模式：先测量每个子节点的主轴固有尺寸，取最大值。
        double maxChildWidth = 0;
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            maxChildWidth = std::max(maxChildWidth, result.intrinsicSize.width());
            maxHeight = std::max(maxHeight, result.intrinsicSize.height());
            if (baselineMode) {
                double b = children[i]->element->baselineOffset(ctx, *children[i]);
                if (b < 0)  // 无基线元素回退为底边对齐（CSS flexbox fallback 语义）
                    b = result.intrinsicSize.height();
                maxBaseline = std::max(maxBaseline, b);
                maxBelowBaseline = std::max(maxBelowBaseline, result.intrinsicSize.height() - b);
            }
        }
        totalWidth = maxChildWidth * static_cast<double>(children.size())
                     + m_space * static_cast<double>(children.size() - 1);
    } else {
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            totalWidth += result.intrinsicSize.width();
            maxHeight = std::max(maxHeight, result.intrinsicSize.height());
            if (baselineMode) {
                double b = children[i]->element->baselineOffset(ctx, *children[i]);
                if (b < 0)  // 无基线元素回退为底边对齐
                    b = result.intrinsicSize.height();
                maxBaseline = std::max(maxBaseline, b);
                maxBelowBaseline = std::max(maxBelowBaseline, result.intrinsicSize.height() - b);
            }
            if (i + 1 < children.size())
                totalWidth += m_space;
        }
    }

    if (baselineMode)
        maxHeight = maxBaseline + maxBelowBaseline;

    QSizeF sz(totalWidth + decoW, maxHeight + decoH);
    return MeasureResult{sz};
}

/// @brief 在给定矩形内水平布局子节点，应用对齐和间距。
/// @param ctx The layout context.
/// @param rect The bounding rectangle assigned to this row.
/// @param node 实例节点，rect 写入 node.rect。
void RowLayout::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    node.rect = rect;

    QRectF cr = contentRect(rect, node.style);

    layoutChildren(ctx, cr, node);
}

/// @brief 在内容区域内定位子节点，处理主轴和交叉轴对齐。
/// @param ctx The layout context.
/// @param contentRect The content area rect (excluding box model decoration).
/// @param node 实例节点。
void RowLayout::layoutChildren(const LayoutContext& ctx, const QRectF& contentRect, Node& node) const
{
    auto& children = node.children;
    if (children.empty())
        return;

    std::vector<QSizeF> childSizes;
    double totalIntrinsicWidth = 0;
    LayoutConstraints childConstraints;
    childConstraints.availableWidth = -1;
    childConstraints.availableHeight = contentRect.height();

    if (m_mainStretch) {
        double maxChildWidth = 0;
        std::vector<bool> hasExplicitWidth;
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            childSizes.push_back(result.intrinsicSize);
            maxChildWidth = std::max(maxChildWidth, result.intrinsicSize.width());
            hasExplicitWidth.push_back(children[i]->style.width >= 0);
        }
        // 用 maxChildWidth 作为约束重新测量（使换行文本重新计算高度）
        childConstraints.availableWidth = maxChildWidth;
        childSizes.clear();
        for (size_t i = 0; i < children.size(); ++i) {
            if (!hasExplicitWidth[i]) {
                LayoutConstraints stretchedConstraints = childConstraints;
                stretchedConstraints.availableWidth = maxChildWidth;
                auto result = children[i]->element->measure(ctx, stretchedConstraints, *children[i]);
                childSizes.push_back(QSizeF(maxChildWidth, result.intrinsicSize.height()));
            } else {
                auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
                childSizes.push_back(result.intrinsicSize);
            }
        }
        totalIntrinsicWidth = maxChildWidth * static_cast<double>(children.size())
                              + m_space * static_cast<double>(children.size() - 1);
    } else {
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            childSizes.push_back(result.intrinsicSize);
            totalIntrinsicWidth += result.intrinsicSize.width();
            if (i + 1 < children.size())
                totalIntrinsicWidth += m_space;
        }
    }

    double extraSpace = contentRect.width() - totalIntrinsicWidth;
    double offsetX = contentRect.x();

    // baseline 交叉轴模式：不做拉伸，先求各子基线与最大基线（无基线回退为子高=底边对齐）
    const bool baselineMode = (m_crossAlign == "baseline");
    std::vector<double> childBaselines;
    double maxBaseline = 0;
    if (baselineMode) {
        childBaselines.reserve(children.size());
        for (size_t i = 0; i < children.size(); ++i) {
            double b = children[i]->element->baselineOffset(ctx, *children[i]);
            if (b < 0)
                b = childSizes[i].height();
            childBaselines.push_back(b);
            maxBaseline = std::max(maxBaseline, b);
        }
    }

    double spacing = m_space;
    if (m_mainAlign == "center") {
        offsetX += extraSpace / 2.0;
    } else if (m_mainAlign == "end") {
        offsetX += extraSpace;
    } else if (m_mainAlign == "space-between" && children.size() > 1) {
        spacing = m_space + extraSpace / (static_cast<double>(children.size()) - 1);
    } else if (m_mainAlign == "space-around" && !children.empty()) {
        double perItem = extraSpace / static_cast<double>(children.size());
        offsetX += perItem / 2.0;
        spacing = m_space + perItem;
    } else if (m_mainAlign == "space-evenly" && !children.empty()) {
        double perGap = extraSpace / static_cast<double>(children.size() + 1);
        offsetX += perGap;
        spacing = m_space + perGap;
    }

    for (size_t i = 0; i < children.size(); ++i) {
        double childWidth = childSizes[i].width();
        double childHeight = childSizes[i].height();

        if (m_crossAlign == "stretch") {
            if (children[i]->style.height < 0)
                childHeight = contentRect.height();
        }

        double childY = contentRect.y();
        if (m_crossAlign == "center") {
            childY = contentRect.y() + (contentRect.height() - childHeight) / 2.0;
        } else if (m_crossAlign == "end") {
            childY = contentRect.y() + contentRect.height() - childHeight;
        } else if (baselineMode) {
            childY = contentRect.y() + (maxBaseline - childBaselines[i]);
        }

        QRectF childRect(offsetX, childY, childWidth, childHeight);
        children[i]->element->layout(ctx, childRect, *children[i]);
        offsetX += childWidth + spacing;
    }
}

} // namespace BroadItem
