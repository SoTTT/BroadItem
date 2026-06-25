#pragma once

#include <QObject>
#include <QPointF>
#include <QMetaObject>

#include <broaditem/reactive/ReactiveBinding.h>

namespace BroadItem {

/// @brief 相对位置跟随绑定。
///
/// 将目标对象（follower）的位置约束为源对象（leader）位置加上一个相对偏移量。
/// 当 leader 移动时，follower 自动保持相对位置同步移动。
/// 当用户拖动 follower 时，偏移量会自动更新，后续 leader 移动时 follower 将保持新的相对距离。
///
/// 内部使用两个 ReactiveBinding：一个负责 leader.pos → follower.pos 的同步；
/// 另一个以观察者模式监听 follower.pos 变化，在用户拖拽时更新偏移量。
class FollowBinding : public QObject {
    Q_OBJECT
public:
    /// @brief 创建 leader 与 follower 之间的相对位置跟随绑定。
    ///
    /// @param leader 自由移动的领导对象，必须继承 QGraphicsObject 或支持 pos 属性。
    /// @param follower 跟随 leader 的目标对象。
    /// @param initialOffset follower 相对于 leader 的初始偏移量。
    /// @param parent 父 QObject。
    /// @return FollowBinding* 新绑定实例；参数无效时返回 nullptr。
    static FollowBinding* create(QObject* leader,
                                 QObject* follower,
                                 const QPointF& initialOffset = QPointF(),
                                 QObject* parent = nullptr);

    /// @brief 销毁绑定，断开所有信号连接。
    void destroy();

    /// @brief 获取当前相对偏移量。
    /// @return leader 指向 follower 的偏移量。
    [[nodiscard]] QPointF offset() const;

    /// @brief 设置新的相对偏移量，并立即同步 follower 位置。
    /// @param offset 新的偏移量。
    void setOffset(const QPointF& offset);

    /// @brief 立即根据当前 leader 位置和偏移量重新同步 follower。
    void sync();

    /// @brief 获取 leader 对象。
    /// @return leader 指针，可能为 nullptr（已销毁）。
    [[nodiscard]] QObject* leader() const;

    /// @brief 获取 follower 对象。
    /// @return follower 指针，可能为 nullptr（已销毁）。
    [[nodiscard]] QObject* follower() const;

private:
    /// @brief 私有构造，通过 create() 工厂创建。
    FollowBinding(QObject* leader,
                  QObject* follower,
                  const QPointF& initialOffset,
                  QObject* parent);

    /// @brief 当 leader 被销毁时清理引用。
    void onLeaderDestroyed();

    /// @brief 当 follower 被销毁时清理引用。
    void onFollowerDestroyed();

    QObject* m_leader;        ///< 领导对象。
    QObject* m_follower;      ///< 跟随对象。
    QPointF m_offset;         ///< 相对偏移量（follower - leader）。
    bool m_updating;          ///< 重入保护标志，防止位置同步循环。

    ReactiveBinding* m_posBinding;                ///< 内部位置同步绑定。
    ReactiveBinding* m_offsetBinding;             ///< follower 拖拽时更新偏移量的观察者绑定。
    QMetaObject::Connection m_leaderDestroyConnection;   ///< leader 销毁连接。
    QMetaObject::Connection m_followerDestroyConnection; ///< follower 销毁连接。
};

} // namespace BroadItem
