#pragma once

#include <broaditem/element/ControlElement.h>
#include <broaditem/expression/Binding.h>

namespace BroadItem {

/// @brief 控制元素，根据属性是否存在/是否为真值有条件地渲染子元素。
class IfHasElement : public ControlElement {
public:
    void parse(const QDomElement& xml) override;
    bool bindsProperty(const QString& name) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    /// @brief 设置用于条件渲染检查的属性名称。
    void setBindProperty(const QString& bind);
    /// @brief 设置是否反转条件（属性不存在时渲染）。
    void setNot(bool notValue);
    /// @brief 设置要有条件渲染的子元素。
    void setChild(ElementPtr child);

    ElementPtr clone() const override;

    /// @brief 根据条件展开为 {克隆的子元素} 或 {}。
    std::vector<ElementPtr> expand(const LayoutContext& ctx) const override;

    /// @brief 测量委托给子元素（当此元素为根元素时使用）。
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    /// @brief 布局委托给子元素（当此元素为根元素时使用）。
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    /// @brief 渲染委托给子元素（当此元素为根元素时使用）。
    void render(QPainter* painter, const LayoutContext& ctx) const override;

private:
    Binding m_binding{"b:prop", QString{}};  ///< Binding for the b:prop attribute.
    bool m_not = false;       ///< If true, invert the condition (render when property does NOT exist).
    ElementPtr m_child;      ///< The child element to conditionally render.

    /// @brief 根据当前上下文评估是否应显示子元素。
    bool shouldShow(const LayoutContext& ctx) const;
};

} // namespace BroadItem
