#pragma once

#include <broaditem/element/RenderableElement.h>
#include <broaditem/compat/Optional.h>
#include <QDomElement>

namespace BroadItem {

/// @brief 支持显式宽度/高度属性的元素基类。
class SizedElement : public RenderableElement {
public:
    virtual ~SizedElement() = default;

    /// @brief 从 XML 解析宽度和高度属性。
    void parse(const QDomElement& xml) override;

    /// @brief 返回显式设置的宽度（未设置时为空）。
    Optional<double> width() const { return m_width; }
    /// @brief 返回显式设置的高度（未设置时为空）。
    Optional<double> height() const { return m_height; }
    /// @brief 如果显式指定了宽度则返回 true。
    bool hasWidth() const { return m_width.has_value(); }
    /// @brief 如果显式指定了高度则返回 true。
    bool hasHeight() const { return m_height.has_value(); }

    /// @brief 返回支持的 XML 属性名集合（"width"、"height"）。
    const QSet<QString>& supportedAttributes() const override {
        static const QSet<QString> attrs = {"width", "height"};
        return attrs;
    }

    /// @brief 将宽度/高度求值进 ResolvedStyle 快照。
    /// @details 先以模板成员值（字面量）填充快照，再应用 b:width/b:height 绑定；
    ///          绑定失配或未绑定时回退为成员值，"未指定"为空 Optional。
    /// @param ctx 布局上下文，用于查找绑定属性值。
    /// @param out 输出快照，width/height 为唯一被写入的字段。
    void resolveSize(const LayoutContext& ctx, ResolvedStyle& out) const;

protected:
    /// @brief 返回有求值路径的绑定属性集合：width/height + 盒模型集合。
    /// @return 静态引用（resolveSize 求值 width/height；盒模型继承自 RenderableElement）。
    const QSet<QString>& resolvedAttributes() const override;

    Optional<double> m_width;   ///< 显式宽度（px），空表示未指定。
    Optional<double> m_height;  ///< 显式高度（px），空表示未指定。
};

} // namespace BroadItem
