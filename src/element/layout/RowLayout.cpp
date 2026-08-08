#include <broaditem/element/layout/RowLayout.h>
#include "LinearLayoutCommon.h"
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 RowLayout 支持的 XML 属性集合。
/// @return 属性名的静态集合引用。
const QSet<QString>& RowLayout::supportedAttributes() const
{
    return LinearLayout::linearLayoutAttributes();
}

/// @brief 解析行布局配置的 XML 属性。
/// @param xml 要解析的 DOM 元素。
void RowLayout::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    validateAttributes(xml);
    // 受保护的字面量读取助手以函数指针传入（受保护访问须在派生类成员上下文中发生）
    LinearLayout::parseAttributes(xml, &hasLiteralAttribute, &literalAttribute,
                                  LinearLayout::Axis::Horizontal,
                                  m_mainAlign, m_crossAlign, m_space, m_mainStretch);
}

/// @brief 测量行：累加子节点宽度，跟踪最大子节点高度。
/// @param ctx 布局上下文。
/// @param constraints 可用宽高约束。
/// @param node 实例节点（children 已物化）。
/// @return 含盒模型装饰的测量尺寸。
MeasureResult RowLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    return LinearLayout::measure(ctx, constraints, node,
                                 LinearLayout::Axis::Horizontal, m_crossAlign, m_space, m_mainStretch);
}

/// @brief 在给定矩形内水平布局子节点，应用对齐和间距。
/// @param ctx 布局上下文。
/// @param rect 分配给此行的矩形。
/// @param node 实例节点，rect 写入 node.rect。
void RowLayout::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    node.rect = rect;

    QRectF cr = contentRect(rect, node.style);

    layoutChildren(ctx, cr, node);
}

/// @brief 在内容区域内定位子节点，处理主轴和交叉轴对齐。
/// @param ctx 布局上下文。
/// @param contentRect 内容区域矩形（不含盒模型装饰）。
/// @param node 实例节点。
void RowLayout::layoutChildren(const LayoutContext& ctx, const QRectF& contentRect, Node& node) const
{
    LinearLayout::layoutChildren(ctx, contentRect, node,
                                 LinearLayout::Axis::Horizontal, m_mainAlign, m_crossAlign, m_space, m_mainStretch);
}

} // namespace BroadItem
