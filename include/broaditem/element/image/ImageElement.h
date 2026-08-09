#pragma once

#include <broaditem/element/SizedElement.h>
#include <QPixmap>
#include <QSet>

namespace BroadItem {

/// @brief ImageElement 的实例节点，持有物化时加载的图像与解析结果。
struct ImageNode : Node {
    QPixmap pixmap;          ///< 物化时加载的图像（失败/未指定时为空）。
    QString srcPath;         ///< 物化时解析的图像路径。
    bool keepAspect = true;  ///< 是否保持宽高比（contain）。
};

/// @brief 自计算图像元素：按 src 加载本地 QPixmap，支持显式尺寸与宽高比保持。
///
/// 语义（已确认决策）：
/// - src 支持字面量与 b:src 绑定（互斥由 parseBindings 既有逻辑裁决）；
/// - 加载失败或未指定 src 时静默渲染为空白（仅盒模型装饰），不绘制占位错误图；
/// - 无显式尺寸时按 pixmap 原始尺寸自计算；单尺寸指定时另一维按宽高比推导。
///
/// 图像缓存：进程级路径键缓存（src/element/image/ImageCache，含互斥锁），
/// 模板不再持有任何 mutable 状态；缓存跨模板共享、永不淘汰。
/// 失败路径入失败表避免重复磁盘加载（告警去重由运行时诊断机制承担）。
class ImageElement : public SizedElement {
public:
    void parse(const QDomElement& xml) override;
    std::unique_ptr<Node> materialize(const LayoutContext& ctx) const override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const override;
    void layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const override;
    void render(QPainter* painter, const LayoutContext& ctx, const Node& node) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return false; }

protected:
    /// @brief 返回实际参与解析/求值的属性集合：SizedElement 集合 + src/keep-aspect。
    const QSet<QString>& resolvedAttributes() const override;

private:
    QString m_srcLiteral;       ///< parse 时读取的字面量 src（绑定时为空，求值走 resolveString）。
    bool m_keepAspect = true;   ///< 字面量 keep-aspect（默认 true）。
};

} // namespace BroadItem
