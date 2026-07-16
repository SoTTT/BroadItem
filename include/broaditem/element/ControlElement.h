#pragma once

#include <broaditem/element/Element.h>
#include <vector>

namespace BroadItem {

/// @brief 不直接参与渲染的控制元素的基类。
///
/// 控制元素（如 ForElement、IfHasElement）在物化（materializeChildren）时
/// 结构性展开为 0..N 个普通元素节点，自身不产生 Node，因此三阶段流水线
/// 永远不会碰到它们——透明性由类型结构保证而非约定。
/// 根据设计文档，控制元素没有装饰器。
class ControlElement : public Element {
public:
    /// @brief 虚析构函数。
    virtual ~ControlElement() = default;

    /// @brief 控制元素不产生实例节点；调用 materialize() 属于编程错误（qFatal）。
    /// 子类只覆盖 materializeChildren()。
    std::unique_ptr<Node> materialize(const LayoutContext& ctx) const override;
};

} // namespace BroadItem
