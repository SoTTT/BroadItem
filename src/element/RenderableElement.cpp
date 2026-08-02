#include <broaditem/element/RenderableElement.h>
#include <QDomElement>
#include <QPainter>

namespace BroadItem {

/// @brief 返回盒模型 XML 属性名的集合。
/// @return 静态集合引用，含 margin、padding、border 与 background 属性名。
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

/// @brief 返回有求值路径的绑定属性集合：resolveStyle 逐项求值全部 17 个盒模型属性。
/// @return boxModelAttributeNames() 的静态引用。
const QSet<QString>& RenderableElement::resolvedAttributes() const
{
    return boxModelAttributeNames();
}

/// @brief 从 XML 元素解析 margin、padding、border 和 background 属性。
/// @param xml 要解析盒模型属性的 DOM 元素。
void RenderableElement::parseBoxModel(const QDomElement& xml)
{
    if (hasLiteralAttribute(xml, "margin"))
        m_margin.left = m_margin.right = m_margin.top = m_margin.bottom = parseDouble(literalAttribute(xml, "margin"));
    if (hasLiteralAttribute(xml, "margin-left"))
        m_margin.left = parseDouble(literalAttribute(xml, "margin-left"));
    if (hasLiteralAttribute(xml, "margin-right"))
        m_margin.right = parseDouble(literalAttribute(xml, "margin-right"));
    if (hasLiteralAttribute(xml, "margin-top"))
        m_margin.top = parseDouble(literalAttribute(xml, "margin-top"));
    if (hasLiteralAttribute(xml, "margin-bottom"))
        m_margin.bottom = parseDouble(literalAttribute(xml, "margin-bottom"));

    if (hasLiteralAttribute(xml, "padding"))
        m_padding.left = m_padding.right = m_padding.top = m_padding.bottom = parseDouble(literalAttribute(xml, "padding"));
    if (hasLiteralAttribute(xml, "padding-left"))
        m_padding.left = parseDouble(literalAttribute(xml, "padding-left"));
    if (hasLiteralAttribute(xml, "padding-right"))
        m_padding.right = parseDouble(literalAttribute(xml, "padding-right"));
    if (hasLiteralAttribute(xml, "padding-top"))
        m_padding.top = parseDouble(literalAttribute(xml, "padding-top"));
    if (hasLiteralAttribute(xml, "padding-bottom"))
        m_padding.bottom = parseDouble(literalAttribute(xml, "padding-bottom"));

    if (hasLiteralAttribute(xml, "border-radius"))
        m_border.radius = parseDouble(literalAttribute(xml, "border-radius"));
    if (hasLiteralAttribute(xml, "border-style"))
        m_border.style = literalAttribute(xml, "border-style");
    if (hasLiteralAttribute(xml, "border-width"))
        m_border.width = parseDouble(literalAttribute(xml, "border-width"));
    if (hasLiteralAttribute(xml, "border-color"))
        m_border.color = parseColor(literalAttribute(xml, "border-color"));

    if (hasLiteralAttribute(xml, "background-color")) {
        m_background.color = parseColor(literalAttribute(xml, "background-color"));
        m_background.enabled = true;
    }
    if (hasLiteralAttribute(xml, "background-radius")) {
        m_background.radius = parseDouble(literalAttribute(xml, "background-radius"));
        m_background.enabled = true;
    }
    if (hasLiteralAttribute(xml, "background-opacity")) {
        m_background.opacity = parseDouble(literalAttribute(xml, "background-opacity"));
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
/// @param painter 目标 QPainter。
/// @param rect 外部矩形（含 margin）。
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
/// @param outerRect 外部边界矩形。
/// @param style 物化时求值得到的盒模型样式快照。
/// @return 内部内容区域矩形。
QRectF RenderableElement::contentRect(const QRectF& outerRect, const ResolvedStyle& style) const
{
    return QRectF(outerRect.x() + style.margin.left + style.border.width + style.padding.left,
                  outerRect.y() + style.margin.top + style.border.width + style.padding.top,
                  std::max(0.0, outerRect.width() - boxModelWidth(style)),
                  std::max(0.0, outerRect.height() - boxModelHeight(style)));
}

} // namespace BroadItem
