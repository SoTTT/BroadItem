#include <broaditem/element/layout/MultiChildContainer.h>
#include <broaditem/element/control/ForElement.h>
#include <broaditem/element/control/IfHasElement.h>
#include <QPainter>
#include <algorithm>

namespace BroadItem {

/// @brief 添加子元素到容器中。
/// @param child 要添加的子元素指针。
void MultiChildContainer::addChild(ElementPtr child)
{
    m_children.push_back(std::move(child));
}

/// @brief 将绑定解析递归传播到所有子元素。
/// @param ctx 布局上下文，包含属性值。
void MultiChildContainer::resolveBindings(const LayoutContext& ctx)
{
    ContainerElement::resolveBindings(ctx);
    for (const auto& child : m_children) {
        if (child)
            child->resolveBindings(ctx);
    }
}

/// @brief 展开控制元素（ForElement、IfHasElement）为具体子元素。
///   ForElement 每个迭代值生成一个克隆实例；
///   IfHasElement 根据条件生成克隆实例或跳过。
/// @param ctx 布局上下文，用于展开控制元素。
/// @return 扁平化后的具体子元素向量。
std::vector<ElementPtr> MultiChildContainer::flattenChildren(const LayoutContext& ctx) const
{
    std::vector<ElementPtr> flat;
    for (const auto& child : m_children) {
        if (!child)
            continue;
        if (auto forEl = std::dynamic_pointer_cast<ForElement>(child)) {
            auto expanded = forEl->expand(ctx);
            flat.insert(flat.end(), expanded.begin(), expanded.end());
        } else if (auto ifEl = std::dynamic_pointer_cast<IfHasElement>(child)) {
            auto expanded = ifEl->expand(ctx);
            flat.insert(flat.end(), expanded.begin(), expanded.end());
        } else {
            flat.push_back(child);
        }
    }
    return flat;
}

/// @brief 渲染盒模型装饰器，然后依次渲染所有展开后的子元素。
/// @param painter 用于渲染的 QPainter。
/// @param ctx 布局上下文。
void MultiChildContainer::render(QPainter* painter, const LayoutContext& ctx) const
{
    ContainerElement::render(painter, ctx);
    for (const auto& child : m_flattened) {
        child->render(painter, ctx);
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
