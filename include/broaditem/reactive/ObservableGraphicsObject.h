#pragma once

#include <QGraphicsObject>

namespace BroadItem {

/// @brief 可观察的 QGraphicsObject 基类，为子类提供位置、缩放、旋转、透明度和可见性变化的信号通知。
///
/// 当 QGraphicsItem 的 ItemChange 事件触发时，本类检测实际数值变化并发射对应信号。
/// BroadItem 继承此类以支持属性变化驱动的响应式重新布局。
class ObservableGraphicsObject : public QGraphicsObject {
    Q_OBJECT
public:
    /// @brief 构造可观察图形对象。
    /// @param parent 父 QGraphicsItem，默认为 nullptr。
    explicit ObservableGraphicsObject(QGraphicsItem* parent = nullptr);

signals:
    /// @brief 当图形对象的位置发生变化时发射。
    void positionChanged(const QPointF& newPos);
    /// @brief 当图形对象的缩放比例发生变化时发射。
    void scaleChanged(qreal newScale);
    /// @brief 当图形对象的旋转角度发生变化时发射。
    void rotationChanged(qreal newRotation);
    /// @brief 当图形对象的透明度发生变化时发射。
    void opacityChanged(qreal newOpacity);
    /// @brief 当图形对象的可见性发生变化时发射。
    void visibilityChanged(bool visible);

protected:
    /// @brief 重写 QGraphicsItem::itemChange，检测属性变化并发射对应信号。
    /// @param change 变化的类型。
    /// @param value 变化的新值。
    /// @return 处理后的值。
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
};

} // namespace BroadItem
