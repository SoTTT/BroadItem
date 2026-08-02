#pragma once

#include <QDomElement>
#include <QRectF>
#include <QSet>
#include <QString>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/core/Node.h>

namespace BroadItem {

/// @brief RowLayout/ColumnLayout 的轴无关共享实现（内部单元，不属公共 API）。
///
/// 行/列布局仅主轴方向不同：parse 的属性集、measure 的约束缩减与
/// main-stretch 双遍测量、layoutChildren 的 main-align 分发布与交叉轴
/// 对齐/拉伸逻辑完全一致，经本单元按轴参数化复用。baseline 交叉轴模式
/// 仅水平轴（RowLayout）生效，垂直轴传入时自动退化为无基线路径。
namespace LinearLayout {

/// @brief 主轴方向：RowLayout 为水平，ColumnLayout 为垂直。
enum class Axis { Horizontal, Vertical };

/// @brief 字面量属性存在性探针（Element::hasLiteralAttribute 的签名）。
using LiteralHas = bool (*)(const QDomElement&, const QString&);
/// @brief 字面量属性取值器（Element::literalAttribute 的签名）。
using LiteralGet = QString (*)(const QDomElement&, const QString&, const QString&);

/// @brief 返回行/列布局支持的 XML 属性集合（含盒模型属性）。
/// @return 属性名的静态集合引用。
const QSet<QString>& linearLayoutAttributes();

/// @brief 解析行/列布局共有属性：main-align、cross-align、space、main-stretch。
///
/// hasLiteral/literal 由派生类以 Element 受保护静态助手的函数指针传入
/// （受保护访问只能在派生类成员上下文中发生）；非法 space/main-stretch
/// 取值按原语义回退（space 保留原值，main-stretch 置 false）。
/// @param xml 要解析的 DOM 元素。
/// @param hasLiteral 字面量属性存在性探针。
/// @param literal 字面量属性取值器。
/// @param mainAlign [out] 主轴对齐（缺省保留调用方原值）。
/// @param crossAlign [out] 交叉轴对齐（缺省保留调用方原值）。
/// @param space [out] 子节点间距。
/// @param mainStretch [out] 是否主轴等距拉伸。
void parseAttributes(const QDomElement& xml, LiteralHas hasLiteral, LiteralGet literal,
                     QString& mainAlign, QString& crossAlign, double& space, bool& mainStretch);

/// @brief 测量：主轴累加（main-stretch 时取最大子主轴尺寸×个数），交叉轴取最大。
/// @param ctx 布局上下文。
/// @param constraints 可用宽高约束。
/// @param node 实例节点（children 已物化）。
/// @param axis 主轴方向。
/// @param crossAlign 交叉轴对齐（"baseline" 仅水平轴生效）。
/// @param space 子节点间距。
/// @param mainStretch 是否主轴等距拉伸。
/// @return 含盒模型装饰的测量尺寸。
MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node,
                      Axis axis, const QString& crossAlign, double space, bool mainStretch);

/// @brief 主轴对齐分发布：按 main-align 推进起始位置并给出最终间距。
/// @param mainAlign 主轴对齐（start/center/end/space-between/space-around/space-evenly）。
/// @param extraSpace 主轴剩余空间。
/// @param count 子节点个数（调用方保证 >= 1）。
/// @param baseSpace 基础间距（space 属性值）。
/// @param startPos [in,out] 主轴起始位置，按对齐策略推进。
/// @param spacing [out] 最终相邻子节点间距。
void distributeMainAxis(const QString& mainAlign, double extraSpace, size_t count,
                        double baseSpace, double& startPos, double& spacing);

/// @brief 在内容区域内定位子节点，处理主轴和交叉轴对齐。
/// @param ctx 布局上下文。
/// @param contentRect 内容区域矩形（不含盒模型装饰）。
/// @param node 实例节点。
/// @param axis 主轴方向。
/// @param mainAlign 主轴对齐。
/// @param crossAlign 交叉轴对齐（"baseline" 仅水平轴生效）。
/// @param space 子节点间距。
/// @param mainStretch 是否主轴等距拉伸。
void layoutChildren(const LayoutContext& ctx, const QRectF& contentRect, Node& node,
                    Axis axis, const QString& mainAlign, const QString& crossAlign,
                    double space, bool mainStretch);

} // namespace LinearLayout

} // namespace BroadItem
