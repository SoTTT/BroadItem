#include <broaditem/layout/ColumnLayout.h>
#include <broaditem/element/SizedElement.h>
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
    if (xml.hasAttribute("main-align"))
        m_mainAlign = xml.attribute("main-align");
    if (xml.hasAttribute("cross-align"))
        m_crossAlign = xml.attribute("cross-align");
    if (xml.hasAttribute("space"))
        validateDouble(xml.attribute("space"), "space", m_space);
    if (xml.hasAttribute("main-stretch")) {
        const QString value = xml.attribute("main-stretch").toLower();
        if (value == "true" || value == "false") {
            m_mainStretch = (value == "true");
        } else {
            qWarning() << "ColumnLayout: 'main-stretch' must be 'true' or 'false', got" << xml.attribute("main-stretch") << "- defaulting to false";
            m_mainStretch = false;
        }
    }
}

/// @brief 创建此列布局的深拷贝，包括所有子元素。
/// @return A new ColumnLayout with cloned properties and children.
ElementPtr ColumnLayout::clone() const
{
    auto copy = std::make_shared<ColumnLayout>();
    copy->m_margin = m_margin;
    copy->m_border = m_border;
    copy->m_background = m_background;
    copy->m_padding = m_padding;
    copy->m_rect = m_rect;
    copy->m_mainAlign = m_mainAlign;
    copy->m_crossAlign = m_crossAlign;
    copy->m_space = m_space;
    copy->m_mainStretch = m_mainStretch;
    for (const auto& child : m_children) {
        if (child)
            copy->m_children.push_back(child->clone());
    }
    return copy;
}

/// @brief 测量列：累加子元素高度，跟踪最大子元素宽度。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @return The measured size including box model decoration.
MeasureResult ColumnLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    double totalHeight = 0;
    double maxWidth = 0;

    LayoutConstraints childConstraints = constraints;
    double decoH = boxModelHeight();
    double decoW = boxModelWidth();

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    m_flattened = flattenChildren(ctx);

    if (m_mainStretch && !m_flattened.empty()) {
        double maxChildHeight = 0;
        for (size_t i = 0; i < m_flattened.size(); ++i) {
            auto result = m_flattened[i]->measure(ctx, childConstraints);
            maxChildHeight = std::max(maxChildHeight, result.intrinsicSize.height());
            maxWidth = std::max(maxWidth, result.intrinsicSize.width());
        }
        totalHeight = maxChildHeight * static_cast<double>(m_flattened.size())
                      + m_space * static_cast<double>(m_flattened.size() - 1);
    } else {
        for (size_t i = 0; i < m_flattened.size(); ++i) {
            auto result = m_flattened[i]->measure(ctx, childConstraints);
            totalHeight += result.intrinsicSize.height();
            maxWidth = std::max(maxWidth, result.intrinsicSize.width());
            if (i + 1 < m_flattened.size())
                totalHeight += m_space;
        }
    }

    QSizeF sz(maxWidth + decoW, totalHeight + decoH);
    return MeasureResult{sz};
}

/// @brief 在给定矩形内垂直布局子元素，应用对齐和间距。
/// @param ctx The layout context.
/// @param rect The bounding rectangle assigned to this column.
void ColumnLayout::layout(const LayoutContext& ctx, const QRectF& rect)
{
    ContainerElement::layout(ctx, rect);

    QRectF cr = contentRect(rect);

    m_flattened = flattenChildren(ctx);
    layoutChildren(ctx, cr);
}

/// @brief 在内容区域内定位子元素，处理主轴和交叉轴对齐。
/// @param ctx The layout context.
/// @param contentRect The content area rect (excluding box model decoration).
void ColumnLayout::layoutChildren(const LayoutContext& ctx, const QRectF& contentRect)
{
    if (m_flattened.empty())
        return;

    std::vector<QSizeF> childSizes;
    double totalIntrinsicHeight = 0;
    LayoutConstraints childConstraints;
    childConstraints.availableWidth = contentRect.width();
    childConstraints.availableHeight = -1;

    if (m_mainStretch) {
        double maxChildHeight = 0;
        std::vector<bool> hasExplicitHeight;
        for (size_t i = 0; i < m_flattened.size(); ++i) {
            auto result = m_flattened[i]->measure(ctx, childConstraints);
            childSizes.push_back(result.intrinsicSize);
            maxChildHeight = std::max(maxChildHeight, result.intrinsicSize.height());
            auto* sized = dynamic_cast<SizedElement*>(m_flattened[i].get());
            hasExplicitHeight.push_back(sized && sized->hasHeight());
        }
        childConstraints.availableHeight = maxChildHeight;
        childSizes.clear();
        for (size_t i = 0; i < m_flattened.size(); ++i) {
            if (!hasExplicitHeight[i]) {
                LayoutConstraints stretchedConstraints = childConstraints;
                stretchedConstraints.availableHeight = maxChildHeight;
                auto result = m_flattened[i]->measure(ctx, stretchedConstraints);
                childSizes.push_back(QSizeF(result.intrinsicSize.width(), maxChildHeight));
            } else {
                auto result = m_flattened[i]->measure(ctx, childConstraints);
                childSizes.push_back(result.intrinsicSize);
            }
        }
        totalIntrinsicHeight = maxChildHeight * static_cast<double>(m_flattened.size())
                               + m_space * static_cast<double>(m_flattened.size() - 1);
    } else {
        for (size_t i = 0; i < m_flattened.size(); ++i) {
            auto result = m_flattened[i]->measure(ctx, childConstraints);
            childSizes.push_back(result.intrinsicSize);
            totalIntrinsicHeight += result.intrinsicSize.height();
            if (i + 1 < m_flattened.size())
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
    } else if (m_mainAlign == "space-between" && m_flattened.size() > 1) {
        spacing = m_space + extraSpace / (static_cast<double>(m_flattened.size()) - 1);
    } else if (m_mainAlign == "space-around" && !m_flattened.empty()) {
        double perItem = extraSpace / static_cast<double>(m_flattened.size());
        offsetY += perItem / 2.0;
        spacing = m_space + perItem;
    } else if (m_mainAlign == "space-evenly" && !m_flattened.empty()) {
        double perGap = extraSpace / static_cast<double>(m_flattened.size() + 1);
        offsetY += perGap;
        spacing = m_space + perGap;
    }

    for (size_t i = 0; i < m_flattened.size(); ++i) {
        double childHeight = childSizes[i].height();
        double childWidth = childSizes[i].width();

        if (m_crossAlign == "stretch") {
            auto* sized = dynamic_cast<SizedElement*>(m_flattened[i].get());
            if (!sized || !sized->hasWidth())
                childWidth = contentRect.width();
        }

        double childX = contentRect.x();
        if (m_crossAlign == "center") {
            childX = contentRect.x() + (contentRect.width() - childWidth) / 2.0;
        } else if (m_crossAlign == "end") {
            childX = contentRect.x() + contentRect.width() - childWidth;
        }

        QRectF childRect(childX, offsetY, childWidth, childHeight);
        m_flattened[i]->layout(ctx, childRect);
        offsetY += childHeight + spacing;
    }
}

} // namespace BroadItem
