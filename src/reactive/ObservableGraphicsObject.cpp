#include <broaditem/reactive/ObservableGraphicsObject.h>

namespace BroadItem {

/// @brief 构造可观察图形对象，启用几何和场景位置变化通知。
/// @param parent 父 QGraphicsItem，默认为 nullptr。
ObservableGraphicsObject::ObservableGraphicsObject(QGraphicsItem* parent)
    : QGraphicsObject(parent)
{
    // 启用几何和场景位置变化标志，使 Qt 内部 xChanged()/yChanged() 信号能够发射
    setFlags(flags() | QGraphicsItem::ItemSendsGeometryChanges
            | QGraphicsItem::ItemSendsScenePositionChanges);
}

/// @brief 重写 QGraphicsItem::itemChange，调用基类实现。
///
/// 信号（xChanged/yChanged/scaleChanged/rotationChanged/opacityChanged/visibleChanged）
/// 均由 Qt 内部 QMetaProperty NOTIFY 机制自动发出，本方法仅透传到基类。
/// @param change 变化的类型。
/// @param value 变化的新值。
/// @return 处理后的值。
QVariant ObservableGraphicsObject::itemChange(GraphicsItemChange change, const QVariant& value)
{
    return QGraphicsObject::itemChange(change, value);
}

} // namespace BroadItem
