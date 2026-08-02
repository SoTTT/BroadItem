#include <broaditem/element/layout/ColumnLayout.h>
#include "LinearLayoutCommon.h"
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 ColumnLayout 支持的 XML 属性集合。
/// @return 属性名的静态集合引用。
const QSet<QString>& ColumnLayout::supportedAttributes() const
{
    return LinearLayout::linearLayoutAttributes();
}

/// @brief 解析列布局配置的 XML 属性。
/// @param xml 要解析的 DOM 元素。
void ColumnLayout::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    validateAttributes(xml);
    // 受保护的字面量读取助手以函数指针传入（受保护访问须在派生类成员上下文中发生）
    LinearLayout::parseAttributes(xml, &hasLiteralAttribute, &literalAttribute,
                                  m_mainAlign, m_crossAlign, m_space, m_mainStretch);
}

/// @brief 测量列：累加子节点高度，跟踪最大子节点宽度。
/// @param ctx 布局上下文。
/// @param constraints 可用宽高约束。
/// @param node 实例节点（children 已物化）。
/// @return 含盒模型装饰的测量尺寸。
MeasureResult ColumnLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    return LinearLayout::measure(ctx, constraints, node,
                                 LinearLayout::Axis::Vertical, m_crossAlign, m_space, m_mainStretch);
}

/// @brief 在给定矩形内垂直布局子节点，应用对齐和间距。
/// @param ctx 布局上下文。
/// @param rect 分配给此列的矩形。
/// @param node 实例节点，rect 写入 node.rect。
void ColumnLayout::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    node.rect = rect;

    QRectF cr = contentRect(rect, node.style);

    layoutChildren(ctx, cr, node);
}

/// @brief 在内容区域内定位子节点，处理主轴和交叉轴对齐。
/// @param ctx 布局上下文。
/// @param contentRect 内容区域矩形（不含盒模型装饰）。
/// @param node 实例节点。
void ColumnLayout::layoutChildren(const LayoutContext& ctx, const QRectF& contentRect, Node& node) const
{
    LinearLayout::layoutChildren(ctx, contentRect, node,
                                 LinearLayout::Axis::Vertical, m_mainAlign, m_crossAlign, m_space, m_mainStretch);
}

} // namespace BroadItem
