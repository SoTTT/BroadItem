#pragma once

#include <broaditem/element/Element.h>
#include <broaditem/element/BoxModel.h>
#include <broaditem/core/ResolvedStyle.h>

namespace BroadItem {

/// @brief 具有可视化表现和盒模型装饰器的元素基类。
class RenderableElement : public Element {
public:
    virtual ~RenderableElement() = default;

    Margin  m_margin;       ///< 元素外边距。
    Border  m_border;       ///< 元素边框。
    Background m_background; ///< 元素背景填充。
    Padding m_padding;      ///< 元素内边距。

    /// @brief 从 XML 解析盒模型属性（边距、边框、背景、内边距）。
    void parseBoxModel(const QDomElement& xml);

    /// @brief 物化时求值：从模板成员 + 上下文解析全部 17 个盒模型属性，写入样式快照。
    ///
    /// 先把 m_margin/m_border/m_background/m_padding 整体拷贝到 out（保证无绑定时
    /// 快照与模板成员默认值一致），再逐项应用绑定覆盖：margin/padding 简写先应用、
    /// 单边属性后应用（与 parseBoxModel 覆盖顺序一致）；background 任一子属性绑定
    /// 有效时置 background.enabled = true（与字面量 parse 语义对齐）。
    /// @param ctx 布局上下文，用于解析数据绑定。
    /// @param out 输出参数，求值结果的唯一写入通道。
    void resolveStyle(const LayoutContext& ctx, ResolvedStyle& out) const;

    /// @brief 渲染盒模型装饰器（背景、边框），样式取自快照。
    void renderBoxModel(QPainter* painter, const QRectF& rect, const ResolvedStyle& style) const;
    /// @brief 从外部矩形减去盒模型装饰器，计算内容区域矩形，样式取自快照。
    QRectF contentRect(const QRectF& outerRect, const ResolvedStyle& style) const;

    /// @brief 返回样式快照对应的水平盒模型总宽度（边距+边框+内边距）。
    static double boxModelWidth(const ResolvedStyle& s) {
        return s.margin.width() + s.border.width * 2 + s.padding.width();
    }
    /// @brief 返回样式快照对应的垂直盒模型总高度（边距+边框+内边距）。
    static double boxModelHeight(const ResolvedStyle& s) {
        return s.margin.height() + s.border.width * 2 + s.padding.height();
    }

    /// @brief 返回支持的盒模型属性名称集合。
    static const QSet<QString>& boxModelAttributeNames();

protected:
    /// @brief 返回有求值路径的绑定属性集合：全部 17 个盒模型属性。
    /// @return boxModelAttributeNames() 的静态引用（resolveStyle 逐项求值）。
    const QSet<QString>& resolvedAttributes() const override;
};

} // namespace BroadItem
