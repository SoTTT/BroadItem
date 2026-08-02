#include <broaditem/element/ContainerElement.h>
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

/// @brief 默认构造函数。
ContainerElement::ContainerElement() = default;

/// @brief 从 XML 元素解析盒模型属性。
/// @param xml 要解析的 DOM 元素。
void ContainerElement::parse(const QDomElement& xml)
{
    parseBoxModel(xml);
}

/// @brief 物化：content 展开为 node->children（0 节点等同空内容）。
/// @param ctx 布局上下文。
/// @return 新创建的实例节点。
std::unique_ptr<Node> ContainerElement::materialize(const LayoutContext& ctx) const
{
    auto node = std::make_unique<Node>();
    node->element = this;
    resolveStyle(ctx, node->style);
    if (m_content)
        node->children = m_content->materializeChildren(ctx);
    return node;
}

/// @brief 测量容器：各子节点垂直堆叠（累加高度、取最大宽度），再加上盒模型装饰。
/// @param ctx 布局上下文。
/// @param constraints 可用宽高约束。
/// @param node 实例节点（children 已物化）。
/// @return 容器的测量尺寸。
MeasureResult ContainerElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    LayoutConstraints childConstraints = constraints;
    double decoW = boxModelWidth(node.style);
    double decoH = boxModelHeight(node.style);

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    double contentW = 0;
    double contentH = 0;
    for (const auto& child : node.children) {
        auto result = child->element->measure(ctx, childConstraints, *child);
        contentW = std::max(contentW, result.intrinsicSize.width());
        contentH += result.intrinsicSize.height();
    }

    QSizeF sz(contentW + decoW, contentH + decoH);
    return MeasureResult{sz};
}

/// @brief 存储分配的矩形，并在内容区域内依次垂直分配各子节点。
/// @param ctx 布局上下文。
/// @param rect 分配给此容器的矩形。
/// @param node 实例节点，rect 写入 node.rect。
void ContainerElement::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    node.rect = rect;
    if (node.children.empty())
        return;

    QRectF cr = contentRect(rect, node.style);
    LayoutConstraints childConstraints;
    childConstraints.availableWidth = cr.width();
    childConstraints.availableHeight = -1;

    double y = cr.y();
    for (const auto& child : node.children) {
        auto result = child->element->measure(ctx, childConstraints, *child);
        double h = result.intrinsicSize.height();
        child->element->layout(ctx, QRectF(cr.x(), y, cr.width(), h), *child);
        y += h;
    }
}

/// @brief 渲染盒模型装饰，然后依次渲染所有子节点。
/// @param painter 目标 QPainter。
/// @param ctx 布局上下文。
/// @param node 实例节点。
void ContainerElement::render(QPainter* painter, const LayoutContext& ctx, const Node& node) const
{
    renderBoxModel(painter, node.rect, node.style);
    for (const auto& child : node.children)
        child->element->render(painter, ctx, *child);
}

/// @brief 检查自身通用绑定或内容元素是否绑定指定属性。
/// @param name 要检查的属性名。
/// @return 自身通用绑定或内容元素绑定该属性时返回 true。
bool ContainerElement::bindsProperty(const QString& name) const
{
    return Element::bindsProperty(name) || (m_content && m_content->bindsProperty(name));
}

/// @brief 设置此容器的单一内容子元素。
/// @param content 要包裹的元素。
void ContainerElement::setContent(ElementPtr content)
{
    m_content = std::move(content);
}

} // namespace BroadItem
