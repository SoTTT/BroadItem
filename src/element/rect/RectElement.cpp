#include <broaditem/element/rect/RectElement.h>
#include <broaditem/compat/QtCompat.h>
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 RectElement 支持的 XML 属性集合。
/// @return 静态集合引用：width/height 与全部盒模型属性（零自有属性）。
const QSet<QString>& RectElement::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{
        "width", "height"
    } + boxModelAttributeNames();
    return attrs;
}

/// @brief 解析色块元素：基类链（width/height + 盒模型）即可，无自有属性。
///
/// resolvedAttributes() 不覆写：继承 SizedElement 集合（盒模型 17 + width/height），
/// 全部属性可绑定，不存在布局策略属性，BI-P-023 门控零调整。
///
/// @param xml 要解析的 DOM 元素。
void RectElement::parse(const QDomElement& xml)
{
    SizedElement::parse(xml);
    parseBoxModel(xml);
    validateAttributes(xml);
}

/// @brief 物化：求值样式与尺寸快照（普通 Node 即可，无自有实例状态）。
/// @param ctx 布局上下文，用于解析数据绑定。
/// @return 新创建的实例节点。
std::unique_ptr<Node> RectElement::materialize(const LayoutContext& ctx) const
{
    auto node = makeUnique<Node>();
    node->element = this;
    resolveStyle(ctx, node->style);
    resolveSize(ctx, node->style);
    return node;
}

/// @brief 测量色块元素：作者声明即固有尺寸。
///
/// 逐维度规则：显式给 → 用显式值；未给 → 取 0（交叉轴维度随后由
/// fillsCrossAxis 恒填充拉满，主轴维度保持 0 不占位）；最后叠加盒模型装饰。
///
/// @param ctx 布局上下文（未使用）。
/// @param constraints 可用宽高约束（未使用；自计算不依赖约束）。
/// @param node 实例节点（普通 Node）。
/// @return 含盒模型装饰的测量尺寸。
MeasureResult RectElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    Q_UNUSED(ctx)
    Q_UNUSED(constraints)
    double w = node.style.width.value_or(0.0);
    double h = node.style.height.value_or(0.0);
    w += boxModelWidth(node.style);
    h += boxModelHeight(node.style);
    return MeasureResult{QSizeF(w, h)};
}

/// @brief 存储分配的矩形（无额外缓存需求）。
/// @param ctx 布局上下文（未使用）。
/// @param rect 分配给此色块元素的矩形。
/// @param node 实例节点。
void RectElement::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    Q_UNUSED(ctx)
    node.rect = rect;
}

/// @brief 渲染色块元素：仅盒模型绘制——background 即色块本体，无内容层。
/// @param painter 目标 QPainter。
/// @param ctx 布局上下文（未使用）。
/// @param node 实例节点。
void RectElement::render(QPainter* painter, const LayoutContext& ctx, const Node& node) const
{
    Q_UNUSED(ctx)
    renderBoxModel(painter, node.rect, node.style);
}

} // namespace BroadItem
