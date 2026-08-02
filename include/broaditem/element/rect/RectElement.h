#pragma once

#include <broaditem/element/SizedElement.h>

namespace BroadItem {

/// @brief 自计算色块元素：作者定尺寸的纯色块。
///
/// 一个部件统一覆盖三种用法（P2-1）：
/// - 分割线：`<rect height="1" background-color="#313244"/>`（column 内宽度自动通栏）；
/// - 方形色块：显式 width/height + background-color；
/// - 圆形指示灯：等边显式尺寸 + background-radius 为边长一半。
///
/// 语义（详见 doc/设计.md「自计算部件」节 `<rect>` 小节）：
/// - 内容是"一笔色块"，固有尺寸取自作者声明——自计算部件固有尺寸的第三种来源
///   （text 取字体度量、image 取图像像素、rect 取作者声明），"内容撑开盒子"不破例；
/// - 逐维度尺寸规则：显式给 → 用显式值（豁免交叉轴填充，交容器 cross-align 定位）；
///   未给 → 交叉轴恒填充（fillsCrossAxis() 为 true，无视容器 cross-align）、主轴取 0；
/// - 零自有属性：width/height + 盒模型全家，全部落在既有可绑定集合，BI-P-023 门控零调整；
/// - 渲染仅盒模型绘制——background 即色块本体，background-radius 画圆角/圆点。
class RectElement : public SizedElement {
public:
    void parse(const QDomElement& xml) override;
    std::unique_ptr<Node> materialize(const LayoutContext& ctx) const override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const override;
    void layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const override;
    void render(QPainter* painter, const LayoutContext& ctx, const Node& node) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return false; }
    bool fillsCrossAxis() const override { return true; }
};

} // namespace BroadItem
