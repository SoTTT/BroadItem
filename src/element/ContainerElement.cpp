#include <broaditem/element/ContainerElement.h>
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

/// @brief 默认构造函数。
ContainerElement::ContainerElement() = default;

/// @brief 从 XML 元素解析盒模型属性。
/// @param xml The DOM element to parse.
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
    if (m_content)
        node->children = m_content->materializeChildren(ctx);
    return node;
}

/// @brief 测量容器：各子节点垂直堆叠（累加高度、取最大宽度），再加上盒模型装饰。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @param node 实例节点（children 已物化）。
/// @return The measured size of the container.
MeasureResult ContainerElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    LayoutConstraints childConstraints = constraints;
    double decoW = boxModelWidth();
    double decoH = boxModelHeight();

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
/// @param ctx The layout context.
/// @param rect The bounding rectangle assigned to this container.
/// @param node 实例节点，rect 写入 node.rect。
void ContainerElement::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    node.rect = rect;
    if (node.children.empty())
        return;

    QRectF cr = contentRect(rect);
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
/// @param painter The QPainter to render onto.
/// @param ctx The layout context.
/// @param node 实例节点。
void ContainerElement::render(QPainter* painter, const LayoutContext& ctx, const Node& node) const
{
    renderBoxModel(painter, node.rect);
    for (const auto& child : node.children)
        child->element->render(painter, ctx, *child);
}

/// @brief 检查内容元素是否绑定指定属性。
/// @param name The property name to check.
/// @return True if content exists and binds the property.
bool ContainerElement::bindsProperty(const QString& name) const
{
    return m_content && m_content->bindsProperty(name);
}

/// @brief 设置此容器的单一内容子元素。
/// @param content The element to wrap.
void ContainerElement::setContent(ElementPtr content)
{
    m_content = std::move(content);
}

} // namespace BroadItem
