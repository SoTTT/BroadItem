#pragma once

#include <QObject>
#include <QPointF>
#include <QMetaObject>

#include <broaditem/reactive/ReactiveBinding.h>

class QGraphicsObject;

namespace BroadItem {

/// @brief 场景坐标相对位置跟随绑定。
///
/// 将目标对象（follower）的位置约束为源对象（leader）在场景坐标系中的位置加上一个相对偏移量。
/// 当 leader 移动或其任意父节点发生变化时，follower 自动保持固定的场景坐标偏移同步移动。
/// 当用户拖动 follower 时，偏移量会自动更新，后续 leader 移动时 follower 将保持新的相对距离。
///
/// 与基于本地 pos 的绑定不同，本绑定使用 scenePos() 计算偏移量，因此 leader 与 follower
/// 可以位于不同的父节点链中，只要它们共享同一个 QGraphicsScene 即可正确跟随。
///
/// 内部使用两个 ReactiveBinding::createScenePosObserver：一个负责监听 leader 的场景位置变化
/// 并触发同步；另一个监听 follower 的场景位置变化，在用户拖拽时更新偏移量。
/// 不同步 visible、opacity 等属性。
class FollowBinding : public QObject {
    Q_OBJECT
public:
    /// @brief 创建 leader 与 follower 之间的场景坐标相对位置跟随绑定。
    ///
    /// @param leader 自由移动的领导对象，必须是 QGraphicsObject 实例且支持 pos 属性。
    /// @param follower 跟随 leader 的目标对象，必须是 QGraphicsObject 实例且支持 pos 属性。
    /// @param initialOffset 保留的初始偏移量参数（当前实现根据创建时 leader 与 follower 的实际场景位置计算偏移）。
    /// @param parent 父 QObject。
    /// @return FollowBinding* 新绑定实例；参数无效时返回 nullptr。
    static FollowBinding* create(QObject* leader,
                                 QObject* follower,
                                 const QPointF& initialOffset = QPointF(),
                                 QObject* parent = nullptr);

    /// @brief 销毁绑定，断开所有信号连接。
    void destroy();

    /// @brief 获取当前场景坐标相对偏移量。
    /// @return leader 指向 follower 的场景坐标偏移量（follower.scenePos() - leader.scenePos()）。
    [[nodiscard]] QPointF offset() const;

    /// @brief 设置新的场景坐标相对偏移量，并立即同步 follower 位置。
    /// @param offset 新的场景坐标偏移量。
    void setOffset(const QPointF& offset);

    /// @brief 立即根据当前 leader 场景位置和偏移量重新同步 follower。
    void sync();

    /// @brief 获取 leader 对象。
    /// @return leader 指针，可能为 nullptr（已销毁）。
    [[nodiscard]] QObject* leader() const;

    /// @brief 获取 follower 对象。
    /// @return follower 指针，可能为 nullptr（已销毁）。
    [[nodiscard]] QObject* follower() const;

private:
    /// @brief 私有构造，通过 create() 工厂创建。
    FollowBinding(QGraphicsObject* leader,
                  QGraphicsObject* follower,
                  const QPointF& initialOffset,
                  QObject* parent);

    /// @brief 当 leader 被销毁时清理引用。
    void onLeaderDestroyed();

    /// @brief 当 follower 被销毁时清理引用。
    void onFollowerDestroyed();

    QGraphicsObject* m_leader;        ///< 领导对象。
    QGraphicsObject* m_follower;      ///< 跟随对象。
    QPointF m_offset;                 ///< 场景坐标相对偏移量（follower.scenePos() - leader.scenePos()）。
    bool m_updating;                  ///< 重入保护标志，防止位置同步循环。

    ReactiveBinding* m_posBinding;                ///< leader 场景位置变化时同步 follower 的绑定。
    ReactiveBinding* m_offsetBinding;             ///< follower 拖拽时更新偏移量的场景位置观察者绑定。
    QMetaObject::Connection m_leaderDestroyConnection;   ///< leader 销毁连接。
    QMetaObject::Connection m_followerDestroyConnection; ///< follower 销毁连接。
};

} // namespace BroadItem
