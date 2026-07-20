#include <broaditem/element/RenderableElement.h>
#include <QDomElement>
#include <QPainter>

namespace BroadItem {

/// @brief 返回盒模型 XML 属性名的集合。
/// @return Reference to a static set of margin, padding, border, and background attribute names.
const QSet<QString>& RenderableElement::boxModelAttributeNames()
{
    static const QSet<QString> attrs = {
        "margin", "margin-left", "margin-right", "margin-top", "margin-bottom",
        "padding", "padding-left", "padding-right", "padding-top", "padding-bottom",
        "border-radius", "border-style", "border-width", "border-color",
        "background-color", "background-radius", "background-opacity"
    };
    return attrs;
}

/// @brief 从 XML 元素解析 margin、padding、border 和 background 属性。
/// @param xml The DOM element to parse box model attributes from.
void RenderableElement::parseBoxModel(const QDomElement& xml)
{
    if (xml.hasAttribute("margin"))
        m_margin.left = m_margin.right = m_margin.top = m_margin.bottom = parseDouble(xml.attribute("margin"));
    if (xml.hasAttribute("margin-left"))
        m_margin.left = parseDouble(xml.attribute("margin-left"));
    if (xml.hasAttribute("margin-right"))
        m_margin.right = parseDouble(xml.attribute("margin-right"));
    if (xml.hasAttribute("margin-top"))
        m_margin.top = parseDouble(xml.attribute("margin-top"));
    if (xml.hasAttribute("margin-bottom"))
        m_margin.bottom = parseDouble(xml.attribute("margin-bottom"));

    if (xml.hasAttribute("padding"))
        m_padding.left = m_padding.right = m_padding.top = m_padding.bottom = parseDouble(xml.attribute("padding"));
    if (xml.hasAttribute("padding-left"))
        m_padding.left = parseDouble(xml.attribute("padding-left"));
    if (xml.hasAttribute("padding-right"))
        m_padding.right = parseDouble(xml.attribute("padding-right"));
    if (xml.hasAttribute("padding-top"))
        m_padding.top = parseDouble(xml.attribute("padding-top"));
    if (xml.hasAttribute("padding-bottom"))
        m_padding.bottom = parseDouble(xml.attribute("padding-bottom"));

    if (xml.hasAttribute("border-radius"))
        m_border.radius = parseDouble(xml.attribute("border-radius"));
    if (xml.hasAttribute("border-style"))
        m_border.style = xml.attribute("border-style");
    if (xml.hasAttribute("border-width"))
        m_border.width = parseDouble(xml.attribute("border-width"));
    if (xml.hasAttribute("border-color"))
        m_border.color = parseColor(xml.attribute("border-color"));

    if (xml.hasAttribute("background-color")) {
        m_background.color = parseColor(xml.attribute("background-color"));
        m_background.enabled = true;
    }
    if (xml.hasAttribute("background-radius")) {
        m_background.radius = parseDouble(xml.attribute("background-radius"));
        m_background.enabled = true;
    }
    if (xml.hasAttribute("background-opacity")) {
        m_background.opacity = parseDouble(xml.attribute("background-opacity"));
        m_background.enabled = true;
    }
}

/// @brief 物化时求值全部盒模型属性，写入样式快照。
///
/// 先整体拷贝模板成员到 out，再按绑定逐项覆盖：
/// - margin/padding：简写绑定有效时四边同值，随后单边绑定逐项覆盖；
/// - border：radius/width 走 resolveDouble，style 走 resolveString，color 走 resolveColor；
/// - background：color/radius/opacity 任一绑定有效时求值写入并置 enabled = true。
/// @param ctx 布局上下文，用于解析数据绑定。
/// @param out 输出参数，求值结果的唯一写入通道。
void RenderableElement::resolveStyle(const LayoutContext& ctx, ResolvedStyle& out) const
{
    out.margin = m_margin;
    out.border = m_border;
    out.background = m_background;
    out.padding = m_padding;

    // margin：简写先应用，单边后应用
    if (boundValue("margin", ctx).isValid()) {
        double v = resolveDouble("margin", ctx, 0);
        out.margin.left = out.margin.right = out.margin.top = out.margin.bottom = v;
    }
    out.margin.left = resolveDouble("margin-left", ctx, out.margin.left);
    out.margin.right = resolveDouble("margin-right", ctx, out.margin.right);
    out.margin.top = resolveDouble("margin-top", ctx, out.margin.top);
    out.margin.bottom = resolveDouble("margin-bottom", ctx, out.margin.bottom);

    // padding：同构
    if (boundValue("padding", ctx).isValid()) {
        double v = resolveDouble("padding", ctx, 0);
        out.padding.left = out.padding.right = out.padding.top = out.padding.bottom = v;
    }
    out.padding.left = resolveDouble("padding-left", ctx, out.padding.left);
    out.padding.right = resolveDouble("padding-right", ctx, out.padding.right);
    out.padding.top = resolveDouble("padding-top", ctx, out.padding.top);
    out.padding.bottom = resolveDouble("padding-bottom", ctx, out.padding.bottom);

    // border
    out.border.radius = resolveDouble("border-radius", ctx, out.border.radius);
    out.border.width = resolveDouble("border-width", ctx, out.border.width);
    out.border.style = resolveString("border-style", ctx, out.border.style);
    out.border.color = resolveColor("border-color", ctx, out.border.color);

    // background：任一子属性绑定有效时求值并启用（与字面量 parse 语义对齐）
    if (boundValue("background-color", ctx).isValid()) {
        out.background.color = resolveColor("background-color", ctx, out.background.color);
        out.background.enabled = true;
    }
    if (boundValue("background-radius", ctx).isValid()) {
        out.background.radius = resolveDouble("background-radius", ctx, out.background.radius);
        out.background.enabled = true;
    }
    if (boundValue("background-opacity", ctx).isValid()) {
        out.background.opacity = resolveDouble("background-opacity", ctx, out.background.opacity);
        out.background.enabled = true;
    }
}

/// @brief 在给定矩形内渲染背景填充和边框描边。
/// @param painter The QPainter to render onto.
/// @param rect The outer rectangle (including margin).
/// @param style 物化时求值得到的盒模型样式快照。
void RenderableElement::renderBoxModel(QPainter* painter, const QRectF& rect, const ResolvedStyle& style) const
{
    double mLeft = style.margin.left;
    double mTop = style.margin.top;
    double mRight = style.margin.right;
    double mBottom = style.margin.bottom;

    QRectF borderRect(rect.x() + mLeft, rect.y() + mTop,
                      std::max(0.0, rect.width() - mLeft - mRight),
                      std::max(0.0, rect.height() - mTop - mBottom));

    QRectF bgRect = borderRect;

    if (style.background.visible()) {
        QColor c = style.background.color;
        c.setAlphaF(style.background.opacity);
        painter->setBrush(c);
        painter->setPen(Qt::NoPen);

        double radius = style.background.radius;
        if (radius > 0)
            painter->drawRoundedRect(bgRect, radius, radius);
        else
            painter->drawRect(bgRect);
    }

    if (style.border.visible()) {
        QPen pen(style.border.color);
        pen.setWidthF(style.border.width);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        double radius = style.border.radius;
        double halfW = style.border.width / 2.0;
        QRectF adjusted(borderRect.x() + halfW, borderRect.y() + halfW,
                        std::max(0.0, borderRect.width() - style.border.width),
                        std::max(0.0, borderRect.height() - style.border.width));

        if (radius > 0)
            painter->drawRoundedRect(adjusted, radius, radius);
        else
            painter->drawRect(adjusted);
    }
}

/// @brief 通过减去 margin、border 和 padding 计算内容区域矩形。
/// @param outerRect The outer bounding rect.
/// @param style 物化时求值得到的盒模型样式快照。
/// @return The inner content rect.
QRectF RenderableElement::contentRect(const QRectF& outerRect, const ResolvedStyle& style) const
{
    return QRectF(outerRect.x() + style.margin.left + style.border.width + style.padding.left,
                  outerRect.y() + style.margin.top + style.border.width + style.padding.top,
                  std::max(0.0, outerRect.width() - boxModelWidth(style)),
                  std::max(0.0, outerRect.height() - boxModelHeight(style)));
}

} // namespace BroadItem
