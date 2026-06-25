#pragma once

#include <QGraphicsObject>
#include <QPen>
#include <QPointF>
#include <QMetaObject>

class QGraphicsScene;

namespace BroadItem {

class ReactiveBinding;
class FollowBinding;

/// @brief 连接线 — 纯场景级视觉项，用于在两个 QGraphicsObject 之间绘制实时跟动的连接线。
///
/// ConnectionLine 通过 ReactiveBinding::createObserver() 监听两端 item 的 pos 属性变化，
/// 当任一端移动时自动重绘连接线。create() 工厂内部透明配对 FollowBinding，
/// 偏移量由创建瞬间的两端位置差自动计算。
///
/// 生命周期：ConnectionLine 析构时显式 destroy() + delete 内部所有 binding 以防止泄漏。
/// 任一端点销毁时，线自动 setVisible(false) 但仍存活，需由用户自行 delete 整线。
///
/// MVP 限制：假定两端点是顶层场景 item（pos == scenePos），不支持嵌套 parent 的 scenePos 映射。
/// 仅跟踪 pos 属性，不跟踪 scale / rotation / opacity / visible。
class ConnectionLine : public QGraphicsObject {
    Q_OBJECT
public:
    static ConnectionLine* create(QGraphicsScene* scene,
                                  QObject* a,
                                  QObject* b,
                                  QObject* parent = nullptr);

    ~ConnectionLine() override;

    [[nodiscard]] QObject* a() const;
    [[nodiscard]] QObject* b() const;
    [[nodiscard]] FollowBinding* followBinding() const;

    void setPen(const QPen& pen);
    [[nodiscard]] QPen pen() const;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

private slots:
    void onAPosChanged();
    void onBPosChanged();

private:
    ConnectionLine(QObject* a, QObject* b, QObject* parent = nullptr);

    QObject* m_a;
    QObject* m_b;
    ReactiveBinding* m_aObserver;
    ReactiveBinding* m_bObserver;
    FollowBinding* m_followBinding;
    QPen m_pen;
    QPointF m_lastAPos;
    QPointF m_lastBPos;

    QMetaObject::Connection m_aDestroyConnection;
    QMetaObject::Connection m_bDestroyConnection;
};

} // namespace BroadItem
