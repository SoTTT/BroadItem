#pragma once

#include <QGraphicsObject>
#include <QColor>

class QGraphicsScene;

namespace BroadItem {

/// @brief 锚点装饰项 — 顶级场景图形项，渲染为实心圆，用于标记被装饰 item 的连接位置。
///
/// AnchorPoint 是纯视觉项，不参与交互：禁用了鼠标按下、选中和移动标志。
/// 构造函数自动将自身添加到 scene 中，并设置 zValue=2 以保证锚点绘制在连接线之上。
///
/// 默认外观：直径 8px，颜色 #555555（深灰），可见。
/// 支持动态修改直径、颜色和可见性，修改后自动重绘。
///
/// QObject parent 应为 AnchorDecorator，QGraphicsItem parent 始终为 nullptr
///（顶级场景 item，pos == scenePos）。
class AnchorPoint : public QGraphicsObject {
    Q_OBJECT
public:
    /// @brief 默认锚点直径（像素）。
    static constexpr qreal kDefaultDiameter = 8.0;

    /// @brief 构造锚点并添加到 scene。
    /// @param scene 目标场景，必须非空。锚点通过 scene->addItem(this) 加入。
    /// @param parent QObject 父对象（通常为 AnchorDecorator）。
    explicit AnchorPoint(QGraphicsScene* scene, QObject* parent = nullptr);

    /// @brief 获取锚点直径。
    /// @return 当前直径（像素）。
    [[nodiscard]] qreal diameter() const;

    /// @brief 设置锚点直径，触发重绘。
    /// @param d 新的直径值（像素）。负值会被取绝对值，且最终结果不会小于 1。
    void setDiameter(qreal d);

    /// @brief 获取锚点填充颜色。
    /// @return 当前颜色。
    [[nodiscard]] QColor color() const;

    /// @brief 设置锚点填充颜色，触发重绘。
    /// @param color 新的填充颜色。
    void setColor(const QColor& color);

    /// @brief 返回锚点的边界矩形（以自身坐标系原点为中心的正方形）。
    /// @return QRectF(-d/2, -d/2, d, d)，其中 d 为当前直径。
    [[nodiscard]] QRectF boundingRect() const override;

    /// @brief 绘制锚点：实心椭圆 + 1px 黑色描边。
    /// @param painter QPainter 实例。
    /// @param option 样式选项（未使用）。
    /// @param widget 目标 widget（未使用）。
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

private:
    qreal m_diameter;   ///< 锚点直径（像素）。
    QColor m_color;     ///< 锚点填充颜色。
};

} // namespace BroadItem
