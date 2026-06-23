#pragma once

#include <broaditem/element/ContainerElement.h>

namespace BroadItem {

/// @brief 多子容器基类，管理多个子元素并支持控制元素的扁平化展开。
class MultiChildContainer : public ContainerElement {
public:
    // Override from ContainerElement
    /// @brief 将绑定解析递归传播到所有子元素。
    void resolveBindings(const LayoutContext& ctx) override;
    /// @brief 渲染盒模型装饰器，然后委托给所有展开后的子元素。
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    /// @brief 检查此容器或任一子元素是否绑定指定属性。
    bool bindsProperty(const QString& name) const override;

    /// @brief 添加子元素到容器中。
    /// @param child 要添加的子元素指针。
    void addChild(ElementPtr child);
    /// @brief 返回扁平化后的子元素列表（控制元素已展开）。
    const std::vector<ElementPtr>& flattenedChildren() const { return m_flattened; }
    /// @brief 返回 true：多子容器可以有子元素。
    bool canHaveChildren() const override { return true; }

    /// @brief Virtual hook for subclasses to validate children before adding.
    /// GridLayout overrides this to validate cell count.
    virtual bool validateChild(const ElementPtr& child) const { Q_UNUSED(child); return true; }

protected:
    std::vector<ElementPtr> m_children;   ///< 原始子元素列表（包含未展开的控制元素）。
    mutable std::vector<ElementPtr> m_flattened; ///< 展开控制元素后的扁平化子元素缓存。

    /// @brief Flatten control elements (ForElement, IfHasElement) into concrete children.
    /// @param ctx 布局上下文，用于展开控制元素。
    /// @return 扁平化后的具体子元素向量。
    std::vector<ElementPtr> flattenChildren(const LayoutContext& ctx) const;
};

} // namespace BroadItem
