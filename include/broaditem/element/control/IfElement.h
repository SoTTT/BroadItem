#pragma once

#include <broaditem/element/ControlElement.h>
#include <broaditem/expression/Binding.h>

namespace BroadItem {

/// @brief 条件控制元素：按数据路径的值断言决定是否物化子元素。
///
/// 三种用法（not 对任意形态整体取反）：
/// - `<if b:prop="warning">`：存在性——属性存在且非 null；
/// - `<if b:prop="status" equals="alarm">`：值比较——与字面量做字符串化比较；
/// - `<if b:prop="status" b:equals="level">`：值比较——比较目标绑定上下文属性，
///   运行时可通过修改该属性改变比较目标（equals 字面量与 b:equals 互斥）。
///
/// 字符串化比较：两侧均取 QVariant::toString() 后区分大小写比较（Qt5 行为：
/// int 3 → "3"；double 3.0 → "3"，尾随零丢失；bool true → "true"）。
/// 绑定比较值不可解析时该次求值条件不成立。
/// not 是结构性修饰符，不支持绑定（b:not 会被警告并忽略）。
class IfElement : public ControlElement {
public:
    void parse(const QDomElement& xml) override;
    bool bindsProperty(const QString& name) const override;
    std::vector<std::unique_ptr<Node>> materializeChildren(const LayoutContext& ctx) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    /// @brief 设置用于条件判断的属性路径。
    void setBindProperty(const QString& bind);
    /// @brief 设置是否反转条件（条件不成立时渲染）。
    void setNot(bool notValue);
    /// @brief 设置字面量比较值（设置后条件为值比较语义）。
    void setEquals(const QString& equals);
    /// @brief 设置要有条件渲染的子元素。
    void setChild(ElementPtr child);

    /// @brief 条件是否有效（b:prop 路径已设置且合法）。
    bool isConditionValid() const { return m_binding.isValid(); }

protected:
    /// @brief 声明 b:equals 有求值路径（经 boundValue 在 shouldShow 中求值）。
    const QSet<QString>& resolvedAttributes() const override;

private:
    Binding m_binding{"b:prop", QString{}};  ///< b:prop 属性绑定的条件路径。
    bool m_not = false;            ///< 为 true 时整体取反（条件不成立时渲染）。
    QString m_equals;              ///< 字面量比较值（m_hasEquals 为 true 时生效）。
    bool m_hasEquals = false;      ///< 区分"未指定 equals"与"equals 为空串"。
    ElementPtr m_child;            ///< 条件成立时物化的子元素。

    /// @brief 根据当前上下文评估是否应显示子元素。
    bool shouldShow(const LayoutContext& ctx) const;
};

} // namespace BroadItem
