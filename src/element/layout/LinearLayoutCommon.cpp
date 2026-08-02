#include "LinearLayoutCommon.h"
#include <broaditem/element/Element.h>
#include <broaditem/element/RenderableElement.h>
#include <algorithm>

namespace BroadItem {
namespace LinearLayout {

namespace {

/// @brief 取尺寸的主轴分量。
double mainExtent(const QSizeF& s, Axis axis)
{
    return axis == Axis::Horizontal ? s.width() : s.height();
}

/// @brief 取尺寸的交叉轴分量。
double crossExtent(const QSizeF& s, Axis axis)
{
    return axis == Axis::Horizontal ? s.height() : s.width();
}

/// @brief 由主轴/交叉轴分量构造尺寸。
QSizeF makeExtent(Axis axis, double main, double cross)
{
    return axis == Axis::Horizontal ? QSizeF(main, cross) : QSizeF(cross, main);
}

/// @brief 由主轴/交叉轴位置与尺寸构造矩形。
QRectF makeRect(Axis axis, double mainPos, double crossPos, double mainSize, double crossSize)
{
    return axis == Axis::Horizontal ? QRectF(mainPos, crossPos, mainSize, crossSize)
                                    : QRectF(crossPos, mainPos, crossSize, mainSize);
}

/// @brief 设置约束的主轴分量。
void setMainConstraint(LayoutConstraints& c, Axis axis, double value)
{
    if (axis == Axis::Horizontal)
        c.availableWidth = value;
    else
        c.availableHeight = value;
}

/// @brief 显式指定的主轴尺寸（style 快照，< 0 表示未显式指定）。
double explicitMainSize(const ResolvedStyle& style, Axis axis)
{
    return axis == Axis::Horizontal ? style.width : style.height;
}

/// @brief 显式指定的交叉轴尺寸（style 快照，< 0 表示未显式指定）。
double explicitCrossSize(const ResolvedStyle& style, Axis axis)
{
    return axis == Axis::Horizontal ? style.height : style.width;
}

} // namespace

const QSet<QString>& linearLayoutAttributes()
{
    static const QSet<QString> attrs = QSet<QString>{"main-align", "cross-align", "space", "main-stretch"}
                                       + RenderableElement::boxModelAttributeNames();
    return attrs;
}

void parseAttributes(const QDomElement& xml, LiteralHas hasLiteral, LiteralGet literal,
                     QString& mainAlign, QString& crossAlign, double& space, bool& mainStretch)
{
    if (hasLiteral(xml, "main-align"))
        mainAlign = literal(xml, "main-align", QString());
    if (hasLiteral(xml, "cross-align"))
        crossAlign = literal(xml, "cross-align", QString());
    if (hasLiteral(xml, "space"))
        Element::validateDouble(literal(xml, "space", QString()), "space", space);
    if (hasLiteral(xml, "main-stretch")) {
        if (!Element::validateBool(literal(xml, "main-stretch", QString()), "main-stretch", mainStretch))
            mainStretch = false;
    }
}

MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node,
                      Axis axis, const QString& crossAlign, double space, bool mainStretch)
{
    double totalMain = 0;
    double maxCross = 0;

    LayoutConstraints childConstraints = constraints;
    const double decoW = RenderableElement::boxModelWidth(node.style);
    const double decoH = RenderableElement::boxModelHeight(node.style);

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    const auto& children = node.children;

    // baseline 交叉轴模式：仅水平轴（RowLayout）生效，内容交叉轴尺寸 = 最大基线 + 最大基线以下高度
    const bool baselineMode = (axis == Axis::Horizontal && crossAlign == "baseline");
    double maxBaseline = 0;
    double maxBelowBaseline = 0;

    if (mainStretch && !children.empty()) {
        // 主轴等距拉伸模式：先测量每个子节点的主轴固有尺寸，取最大值。
        double maxChildMain = 0;
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            maxChildMain = std::max(maxChildMain, mainExtent(result.intrinsicSize, axis));
            maxCross = std::max(maxCross, crossExtent(result.intrinsicSize, axis));
            if (baselineMode) {
                double b = children[i]->element->baselineOffset(ctx, *children[i]);
                if (b < 0)  // 无基线元素回退为底边对齐（CSS flexbox fallback 语义）
                    b = crossExtent(result.intrinsicSize, axis);
                maxBaseline = std::max(maxBaseline, b);
                maxBelowBaseline = std::max(maxBelowBaseline, crossExtent(result.intrinsicSize, axis) - b);
            }
        }
        totalMain = maxChildMain * static_cast<double>(children.size())
                    + space * static_cast<double>(children.size() - 1);
    } else {
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            totalMain += mainExtent(result.intrinsicSize, axis);
            maxCross = std::max(maxCross, crossExtent(result.intrinsicSize, axis));
            if (baselineMode) {
                double b = children[i]->element->baselineOffset(ctx, *children[i]);
                if (b < 0)  // 无基线元素回退为底边对齐
                    b = crossExtent(result.intrinsicSize, axis);
                maxBaseline = std::max(maxBaseline, b);
                maxBelowBaseline = std::max(maxBelowBaseline, crossExtent(result.intrinsicSize, axis) - b);
            }
            if (i + 1 < children.size())
                totalMain += space;
        }
    }

    if (baselineMode)
        maxCross = maxBaseline + maxBelowBaseline;

    const double decoMain = axis == Axis::Horizontal ? decoW : decoH;
    const double decoCross = axis == Axis::Horizontal ? decoH : decoW;
    return MeasureResult{makeExtent(axis, totalMain + decoMain, maxCross + decoCross)};
}

void distributeMainAxis(const QString& mainAlign, double extraSpace, size_t count,
                        double baseSpace, double& startPos, double& spacing)
{
    spacing = baseSpace;
    if (mainAlign == "center") {
        startPos += extraSpace / 2.0;
    } else if (mainAlign == "end") {
        startPos += extraSpace;
    } else if (mainAlign == "space-between" && count > 1) {
        spacing = baseSpace + extraSpace / (static_cast<double>(count) - 1);
    } else if (mainAlign == "space-around") {
        double perItem = extraSpace / static_cast<double>(count);
        startPos += perItem / 2.0;
        spacing = baseSpace + perItem;
    } else if (mainAlign == "space-evenly") {
        double perGap = extraSpace / static_cast<double>(count + 1);
        startPos += perGap;
        spacing = baseSpace + perGap;
    }
}

void layoutChildren(const LayoutContext& ctx, const QRectF& contentRect, Node& node,
                    Axis axis, const QString& mainAlign, const QString& crossAlign,
                    double space, bool mainStretch)
{
    auto& children = node.children;
    if (children.empty())
        return;

    std::vector<QSizeF> childSizes;
    double totalIntrinsicMain = 0;
    LayoutConstraints childConstraints;
    // 主轴不设约束，交叉轴约束为内容区交叉轴尺寸
    if (axis == Axis::Horizontal) {
        childConstraints.availableWidth = -1;
        childConstraints.availableHeight = contentRect.height();
    } else {
        childConstraints.availableWidth = contentRect.width();
        childConstraints.availableHeight = -1;
    }

    if (mainStretch) {
        double maxChildMain = 0;
        std::vector<bool> hasExplicitMain;
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            childSizes.push_back(result.intrinsicSize);
            maxChildMain = std::max(maxChildMain, mainExtent(result.intrinsicSize, axis));
            hasExplicitMain.push_back(explicitMainSize(children[i]->style, axis) >= 0);
        }
        // 用 maxChildMain 作为主轴约束重新测量（使换行文本重新计算交叉轴尺寸）；
        // 显式指定主轴尺寸的子元素不被拉伸，但仍按同一约束测量
        setMainConstraint(childConstraints, axis, maxChildMain);
        childSizes.clear();
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            if (!hasExplicitMain[i])
                childSizes.push_back(makeExtent(axis, maxChildMain, crossExtent(result.intrinsicSize, axis)));
            else
                childSizes.push_back(result.intrinsicSize);
        }
        totalIntrinsicMain = maxChildMain * static_cast<double>(children.size())
                             + space * static_cast<double>(children.size() - 1);
    } else {
        for (size_t i = 0; i < children.size(); ++i) {
            auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
            childSizes.push_back(result.intrinsicSize);
            totalIntrinsicMain += mainExtent(result.intrinsicSize, axis);
            if (i + 1 < children.size())
                totalIntrinsicMain += space;
        }
    }

    const double contentMainPos = axis == Axis::Horizontal ? contentRect.x() : contentRect.y();
    const double contentMainSize = axis == Axis::Horizontal ? contentRect.width() : contentRect.height();
    const double contentCrossPos = axis == Axis::Horizontal ? contentRect.y() : contentRect.x();
    const double contentCrossSize = axis == Axis::Horizontal ? contentRect.height() : contentRect.width();

    double extraSpace = contentMainSize - totalIntrinsicMain;
    double offset = contentMainPos;

    // baseline 交叉轴模式：仅水平轴生效；不做拉伸，先求各子基线与最大基线（无基线回退为底边对齐）
    const bool baselineMode = (axis == Axis::Horizontal && crossAlign == "baseline");
    std::vector<double> childBaselines;
    double maxBaseline = 0;
    if (baselineMode) {
        childBaselines.reserve(children.size());
        for (size_t i = 0; i < children.size(); ++i) {
            double b = children[i]->element->baselineOffset(ctx, *children[i]);
            if (b < 0)
                b = crossExtent(childSizes[i], axis);
            childBaselines.push_back(b);
            maxBaseline = std::max(maxBaseline, b);
        }
    }

    double spacing;
    distributeMainAxis(mainAlign, extraSpace, children.size(), space, offset, spacing);

    for (size_t i = 0; i < children.size(); ++i) {
        double childMain = mainExtent(childSizes[i], axis);
        double childCross = crossExtent(childSizes[i], axis);

        // 交叉轴拉伸：cross-align=stretch 或子元素自报恒填充（fillsCrossAxis）；
        // 内层显式交叉轴尺寸豁免判断不受 fillsCrossAxis 影响
        if (crossAlign == "stretch" || children[i]->element->fillsCrossAxis()) {
            if (explicitCrossSize(children[i]->style, axis) < 0)
                childCross = contentCrossSize;
        }

        double crossPos = contentCrossPos;
        if (crossAlign == "center") {
            crossPos = contentCrossPos + (contentCrossSize - childCross) / 2.0;
        } else if (crossAlign == "end") {
            crossPos = contentCrossPos + contentCrossSize - childCross;
        } else if (baselineMode) {
            crossPos = contentCrossPos + (maxBaseline - childBaselines[i]);
        }

        children[i]->element->layout(ctx, makeRect(axis, offset, crossPos, childMain, childCross), *children[i]);
        offset += childMain + spacing;
    }
}

} // namespace LinearLayout
} // namespace BroadItem
