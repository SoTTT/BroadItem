#include <broaditem/element/ControlElement.h>
#include <QDebug>

namespace BroadItem {

// 控制元素（for、if-has）不参与测量/布局/渲染，也不产生实例节点。
// 它们只在物化阶段经 materializeChildren() 展开后结构性消失。

/// @brief 控制元素永不物化为单个节点；此实现不可达。
std::unique_ptr<Node> ControlElement::materialize(const LayoutContext& ctx) const
{
    Q_UNUSED(ctx)
    qFatal("ControlElement::materialize: control elements never produce nodes (use materializeChildren)");
    return nullptr;  // unreachable
}

} // namespace BroadItem
