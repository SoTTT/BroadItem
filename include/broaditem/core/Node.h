#pragma once

#include <QRectF>
#include <broaditem/core/ResolvedStyle.h>
#include <memory>
#include <vector>

namespace BroadItem {

class Element;

/// @brief 实例层节点：三阶段流水线（测量、布局、渲染）作用的哑数据袋。
///
/// 模板层 Element 在 parse 后不可变；每实例状态（布局矩形、物化后的子节点）
/// 全部存放在 Node 树中。控制元素（for、if）在物化时结构性消失，
/// 因此实例树中的 element 指针只指向可渲染元素。
struct Node {
    const Element* element = nullptr;  ///< 产生此节点的模板（非拥有；模板树由 Frame/Registry 持有，生命周期覆盖 Node）。
    QRectF rect;                       ///< 布局阶段的输出矩形。
    std::vector<std::unique_ptr<Node>> children;  ///< 物化后的子节点（控制元素已展开）。
    ResolvedStyle style;              ///< 物化时求值的样式快照；仅可渲染元素产生节点时有意义。

    virtual ~Node() = default;
};

} // namespace BroadItem
