#pragma once

#include "../ControlElement.h"
#include "broaditem/Binding.h"

namespace BroadItem {

/// @brief 控制元素，在运行时展开为多个克隆实例，每个绑定可迭代值一个，并应用插值。
class ForElement : public ControlElement {
public:
    void parse(const QDomElement& xml) override;
    bool bindsProperty(const QString& name) const override;

    /// @brief 设置要迭代的属性名称。
    void setBindProperty(const QString& bind);
    /// @brief 设置每次迭代要克隆的模板元素。
    void setTemplate(ElementPtr templ);
    /// @brief 设置 :as 别名变量名。
    void setAsVariable(const QString& v) { m_asVariable = v; }
    /// @brief 获取模板元素。
    ElementPtr templateElement() const { return m_template; }

    ElementPtr clone() const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    /// @brief 将此 for 元素展开为克隆元素实例列表，每个绑定值一个。
    std::vector<ElementPtr> expand(const LayoutContext& ctx) const override;

    /// @brief 测量委托给展开后的子元素（当此元素为根元素时使用）。
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    /// @brief 布局委托给展开后的子元素（当此元素为根元素时使用）。
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    /// @brief 渲染委托给展开后的子元素（当此元素为根元素时使用）。
    void render(QPainter* painter, const LayoutContext& ctx) const override;

private:
    Binding m_binding{":of", QString{}}; ///< Binding for the :of attribute (data source).
    QString m_asVariable;   ///< The alias variable name bound to each iteration value.
    ElementPtr m_template;  ///< The template element to clone for each iteration.
};

} // namespace BroadItem
