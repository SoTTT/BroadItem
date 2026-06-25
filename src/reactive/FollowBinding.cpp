#include <broaditem/reactive/FollowBinding.h>
#include <broaditem/reactive/ReactiveProperty.h>

#include <QDebug>

namespace BroadItem {

/// @brief 创建 leader 与 follower 之间的相对位置跟随绑定。
///
/// 校验 leader 和 follower 非空，且均支持 pos 属性。参数无效时返回 nullptr。
/// @param leader 自由移动的领导对象。
/// @param follower 跟随 leader 的目标对象。
/// @param initialOffset follower 相对于 leader 的初始偏移量。
/// @param parent 父 QObject。
/// @return FollowBinding* 新绑定实例；参数无效时返回 nullptr。
FollowBinding* FollowBinding::create(QObject* leader,
                                     QObject* follower,
                                     const QPointF& initialOffset,
                                     QObject* parent)
{
    if (!leader) {
        qWarning() << "FollowBinding::create: leader is null";
        return nullptr;
    }

    if (!follower) {
        qWarning() << "FollowBinding::create: follower is null";
        return nullptr;
    }

    if (!leader->property(Property::Pos.toLatin1().constData()).isValid()) {
        qWarning() << "FollowBinding::create: leader does not have pos property" << leader;
        return nullptr;
    }

    if (!follower->property(Property::Pos.toLatin1().constData()).isValid()) {
        qWarning() << "FollowBinding::create: follower does not have pos property" << follower;
        return nullptr;
    }

    return new FollowBinding(leader, follower, initialOffset, parent);
}

/// @brief 私有构造，设置内部 ReactiveBinding 和 follower 拖拽监听。
FollowBinding::FollowBinding(QObject* leader,
                             QObject* follower,
                             const QPointF& initialOffset,
                             QObject* parent)
    : QObject(parent)
    , m_leader(leader)
    , m_follower(follower)
    , m_offset(initialOffset)
    , m_updating(false)
    , m_posBinding(nullptr)
    , m_offsetBinding(nullptr)
{
    // 使用 ReactiveBinding 将 leader.pos 同步到 follower.pos，并应用偏移量变换。
    auto posTransform = [this](const QVariant& value) -> QVariant {
        return value.toPointF() + m_offset;
    };

    m_posBinding = ReactiveBinding::create(m_leader, Property::Pos,
                                           m_follower, Property::Pos,
                                           posTransform);

    // 使用 ReactiveBinding 的观察者模式监听 follower.pos 变化，用于捕获用户拖拽并更新偏移量。
    auto offsetObserver = [this](const QVariant& value) -> QVariant {
        if (!m_leader || !m_follower || m_updating) {
            return {};
        }
        QPointF leaderPos = m_leader->property(Property::Pos.toLatin1().constData()).toPointF();
        m_offset = value.toPointF() - leaderPos;
        return {};
    };

    m_offsetBinding = ReactiveBinding::createObserver(m_follower, Property::Pos,
                                                      offsetObserver);

    // 监听对象销毁。
    if (m_leader) {
        m_leaderDestroyConnection = connect(m_leader, &QObject::destroyed,
                                            this, &FollowBinding::onLeaderDestroyed);
    }
    if (m_follower) {
        m_followerDestroyConnection = connect(m_follower, &QObject::destroyed,
                                              this, &FollowBinding::onFollowerDestroyed);
    }
}

/// @brief 销毁绑定，断开所有信号连接并清理内部绑定。
void FollowBinding::destroy()
{
    if (m_posBinding != nullptr) {
        m_posBinding->destroy();
        delete m_posBinding;
        m_posBinding = nullptr;
    }

    if (m_offsetBinding != nullptr) {
        m_offsetBinding->destroy();
        delete m_offsetBinding;
        m_offsetBinding = nullptr;
    }

    disconnect(m_leaderDestroyConnection);
    disconnect(m_followerDestroyConnection);

    m_leader = nullptr;
    m_follower = nullptr;
}

/// @brief 获取当前相对偏移量。
QPointF FollowBinding::offset() const
{
    return m_offset;
}

/// @brief 设置新的相对偏移量，并立即同步 follower 位置。
void FollowBinding::setOffset(const QPointF& offset)
{
    m_offset = offset;
    sync();
}

/// @brief 立即根据当前 leader 位置和偏移量重新同步 follower。
void FollowBinding::sync()
{
    if (!m_leader || !m_follower || m_updating) {
        return;
    }

    m_updating = true;
    QPointF leaderPos = m_leader->property(Property::Pos.toLatin1().constData()).toPointF();
    m_follower->setProperty(Property::Pos.toLatin1().constData(), leaderPos + m_offset);
    m_updating = false;
}

/// @brief 获取 leader 对象。
QObject* FollowBinding::leader() const
{
    return m_leader;
}

/// @brief 获取 follower 对象。
QObject* FollowBinding::follower() const
{
    return m_follower;
}

/// @brief leader 销毁时清理引用并禁用绑定。
void FollowBinding::onLeaderDestroyed()
{
    m_leader = nullptr;
    destroy();
}

/// @brief follower 销毁时清理引用并禁用绑定。
void FollowBinding::onFollowerDestroyed()
{
    m_follower = nullptr;
    destroy();
}

} // namespace BroadItem
