#pragma once

#include <QGraphicsObject>

namespace BroadItem {

/// @brief 可观察的 QGraphicsObject 基类，为 Qt 内部通过 QMetaProperty NOTIFY 机制驱动信号提供基础。
///
/// 本类启用 ItemSendsGeometryChanges 和 ItemSendsScenePositionChanges 标志，
/// 使得 Qt 内部 xChanged() 和 yChanged() 信号能够正常发射。
/// 其他属性（scale、rotation、opacity、visible）的 NOTIFY 信号由 QGraphicsObject/QGraphicsItem
/// 内部的 QMetaProperty 机制自行发出，不需要本类额外处理。
/// BroadItem 继承此类以支持响应式绑定。
class ObservableGraphicsObject : public QGraphicsObject {
    Q_OBJECT
public:
    /// @brief 构造可观察图形对象。
    /// @param parent 父 QGraphicsItem，默认为 nullptr。
    explicit ObservableGraphicsObject(QGraphicsItem* parent = nullptr);

protected:
    /// @brief 重写 QGraphicsItem::itemChange，调用基类实现。
    ///
    /// 保留此重写以保持虚函数链完整，实际信号（xChanged/yChanged等）由 Qt 内部发出。
    /// @param change 变化的类型。
    /// @param value 变化的新值。
    /// @return 处理后的值。
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
};

} // namespace BroadItem
