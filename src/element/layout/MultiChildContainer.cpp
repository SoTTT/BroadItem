#include <broaditem/element/layout/MultiChildContainer.h>
#include <algorithm>

namespace BroadItem {

/// @brief 添加子元素到容器中。
/// @param child 要添加的子元素指针。
void MultiChildContainer::addChild(ElementPtr child)
{
    m_children.push_back(std::move(child));
}

/// @brief 物化：对每个模板子元素调 materializeChildren() 拼接进 node->children。
///   ForElement 每个迭代值产生一组节点；IfElement 根据条件产生节点或跳过。
/// @param ctx 布局上下文。
/// @return 新创建的实例节点。
std::unique_ptr<Node> MultiChildContainer::materialize(const LayoutContext& ctx) const
{
    auto node = std::make_unique<Node>();
    node->element = this;
    resolveStyle(ctx, node->style);
    materializeChildrenInto(ctx, *node);
    return node;
}

/// @brief 将所有模板子元素物化并拼接进 node.children。
/// @param ctx 布局上下文。
/// @param node 目标实例节点。
void MultiChildContainer::materializeChildrenInto(const LayoutContext& ctx, Node& node) const
{
    for (const auto& child : m_children) {
        if (!child)
            continue;
        auto nodes = child->materializeChildren(ctx);
        for (auto& n : nodes)
            node.children.push_back(std::move(n));
    }
}

/// @brief 检查此容器或任一子元素是否绑定指定属性。
/// @param name 要检查的属性名。
/// @return 如果容器或任何子元素绑定该属性则返回 true。
bool MultiChildContainer::bindsProperty(const QString& name) const
{
    if (ContainerElement::bindsProperty(name))
        return true;
    return std::any_of(m_children.begin(), m_children.end(),
        [&](const auto& child) { return child && child->bindsProperty(name); });
}

} // namespace BroadItem
