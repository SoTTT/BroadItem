#include <broaditem/reactive/ObservableGraphicsObject.h>

namespace BroadItem {

/// @brief 构造可观察图形对象，启用几何和场景位置变化通知。
/// @param parent 父 QGraphicsItem，默认为 nullptr。
ObservableGraphicsObject::ObservableGraphicsObject(QGraphicsItem* parent)
    : QGraphicsObject(parent)
{
    setFlags(flags() | QGraphicsItem::ItemSendsGeometryChanges
            | QGraphicsItem::ItemSendsScenePositionChanges);
}

/// @brief 重写 QGraphicsItem::itemChange，检测属性变化并发射对应信号。
///
/// 当 QGraphicsItem 的属性发生变化时，此方法根据变化类型发射对应的信号，
/// 然后调用基类 QGraphicsObject::itemChange 完成默认处理。
/// @param change 变化的类型。
/// @param value 变化的新值。
/// @return 处理后的值。
QVariant ObservableGraphicsObject::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case ItemPositionHasChanged:
        emit positionChanged(value.toPointF());
        break;
    case ItemScaleHasChanged:
        emit scaleChanged(value.toDouble());
        break;
    case ItemRotationHasChanged:
        emit rotationChanged(value.toDouble());
        break;
    case ItemOpacityHasChanged:
        emit opacityChanged(value.toDouble());
        break;
    case ItemVisibleHasChanged:
        emit visibilityChanged(value.toBool());
        break;
    default:
        break;
    }
    return QGraphicsObject::itemChange(change, value);
}

} // namespace BroadItem
