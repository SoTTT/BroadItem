#pragma once

#include <QGraphicsObject>
#include <QPen>
#include <QPointF>
#include <QMetaObject>

class QGraphicsScene;

namespace BroadItem {

class ReactiveBinding;
class FollowBinding;

/// @brief 连接线 — 纯场景级视觉项，在两个 QGraphicsObject 之间绘制实时跟动的连接线。
///
/// 连接线自身为场景顶级图元，boundingRect() 与 paint() 均使用场景坐标。
/// 两端点通过 ReactiveBinding::createScenePosObserver() 监听其 scenePos() 变化，
/// 包括端点自身及其所有 QGraphicsObject 祖先的位置、变换与可见性变化，
/// 因此端点可位于任意深度的嵌套 parent 链中。
/// create() 工厂内部使用 FollowBinding 配对，偏移量由创建瞬间两端 scenePos() 的差值决定。
///
/// 生命周期：端点销毁或离开场景时，线自动 setVisible(false) 隐藏；
/// 整线仍保持存活，由调用方负责 delete。析构时显式 destroy() + delete
/// 内部所有 binding，避免连接泄漏。
class ConnectionLine : public QGraphicsObject {
    Q_OBJECT
public:
    /// @brief 创建一条连接 a 与 b 的场景级连接线。
    ///
    /// 内部创建 ConnectionLine、将其加入 scene，并建立 FollowBinding 与
    /// ReactiveBinding::createScenePosObserver() 监听。偏移量由创建瞬间两端
    /// scenePos() 的差值决定。
    ///
    /// @param scene 目标场景，必须非空且 a、b 均已加入该场景。
    /// @param a 端点 A，必须为非空的 QGraphicsObject 且拥有 pos 属性。
    /// @param b 端点 B，必须为非空的 QGraphicsObject 且拥有 pos 属性。
    /// @param parent 可选的 QObject 父对象。
    /// @return ConnectionLine* 新实例；参数无效时返回 nullptr。
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
    /// @brief 端点 A 的场景位置变化回调。
    ///
    /// 更新最近记录的场景坐标并重绘；若端点已销毁或离开场景则隐藏线。
    void onAPosChanged();

    /// @brief 端点 B 的场景位置变化回调。
    ///
    /// 更新最近记录的场景坐标并重绘；若端点已销毁或离开场景则隐藏线。
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
