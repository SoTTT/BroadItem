#pragma once

#include <broaditem/element/ControlElement.h>
#include <broaditem/expression/Binding.h>

namespace BroadItem {

/// @brief 控制元素，物化时按迭代列表逐项展开模板并拼接实例节点。
class ForElement : public ControlElement {
public:
    void parse(const QDomElement& xml) override;
    bool bindsProperty(const QString& name) const override;
    std::vector<std::unique_ptr<Node>> materializeChildren(const LayoutContext& ctx) const override;

    /// @brief 设置要迭代的属性名称。
    void setBindProperty(const QString& bind);
    /// @brief 设置每次迭代要物化的模板元素。
    void setTemplate(ElementPtr templ);
    /// @brief 设置 b:as 别名变量名。
    void setAsVariable(const QString& v) { m_asVariable = v; }
    /// @brief 获取模板元素。
    ElementPtr templateElement() const { return m_template; }

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

private:
    Binding m_binding{"b:of", QString{}}; ///< Binding for the b:of attribute (data source).
    QString m_asVariable;   ///< The alias variable name bound to each iteration value.
    ElementPtr m_template;  ///< The template element to materialize for each iteration.
};

} // namespace BroadItem
