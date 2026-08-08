#pragma once

#include <broaditem/element/ContainerElement.h>

namespace BroadItem {

/// @brief 多子容器基类，管理多个模板子元素。
///
/// 物化时对每个模板子元素调 materializeChildren() 拼接进 node->children，
/// 控制元素（for、if）在此结构性展开；三阶段遍历 node.children。
class MultiChildContainer : public ContainerElement {
public:
    /// @brief 物化：创建裸 Node 并拼接所有模板子元素的物化结果。
    std::unique_ptr<Node> materialize(const LayoutContext& ctx) const override;
    /// @brief 检查此容器或任一子元素是否绑定指定属性。
    bool bindsProperty(const QString& name) const override;

    /// @brief 添加子元素到容器中。
    /// @param child 要添加的子元素指针。
    void addChild(ElementPtr child);
    /// @brief 返回 true：多子容器可以有子元素。
    bool canHaveChildren() const override { return true; }
    /// @brief 解析期挂载：追加到模板子元素列表（column/row/grid 共用）。
    void addParsedChild(const ElementPtr& child) override;

protected:
    std::vector<ElementPtr> m_children;   ///< 模板子元素列表（含未展开的控制元素）。

    /// @brief 将所有模板子元素物化并拼接进 node.children。
    /// @param ctx 布局上下文。
    /// @param node 目标实例节点。
    void materializeChildrenInto(const LayoutContext& ctx, Node& node) const;
};

} // namespace BroadItem
