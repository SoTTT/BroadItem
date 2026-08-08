/// @file FollowBinding.cpp
/// @brief FollowBinding 实现 —— 场景坐标相对位置跟随绑定。

#include <broaditem/reactive/FollowBinding.h>
#include <broaditem/reactive/ReactiveBinding.h>
#include <broaditem/reactive/ReactiveProperty.h>

#include "ReentrancyGuard.h"

#include <QDebug>
#include <QGraphicsObject>

namespace BroadItem {

/// @brief 创建 leader 与 follower 之间的场景坐标相对位置跟随绑定。
///
/// 校验 leader 和 follower 非空，均为 QGraphicsObject 实例，且均支持 pos 属性。参数无效时返回 nullptr。
/// initialOffset 非零时直接采用；为零点（缺省值）时根据创建时 leader 与 follower 的实际场景位置计算：
/// m_offset = follower->scenePos() - leader->scenePos()。
/// @param leader 自由移动的领导对象。
/// @param follower 跟随 leader 的目标对象。
/// @param initialOffset 初始偏移量；为零点时按实际场景位置自动计算。
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

    auto* leaderObj = qobject_cast<QGraphicsObject*>(leader);
    if (!leaderObj) {
        qWarning() << "FollowBinding::create: leader is not a QGraphicsObject" << leader;
        return nullptr;
    }

    auto* followerObj = qobject_cast<QGraphicsObject*>(follower);
    if (!followerObj) {
        qWarning() << "FollowBinding::create: follower is not a QGraphicsObject" << follower;
        return nullptr;
    }

    // initialOffset 非零时采用调用方指定值；零点（缺省）按实际场景位置计算。
    const QPointF offset = initialOffset.isNull()
                               ? followerObj->scenePos() - leaderObj->scenePos()
                               : initialOffset;
    return new FollowBinding(leaderObj, followerObj, offset, parent);
}

/// @brief 私有构造，设置内部场景位置监听和 follower 拖拽监听。
FollowBinding::FollowBinding(QGraphicsObject* leader,
                             QGraphicsObject* follower,
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
    // 监听 leader 的场景位置变化，变化时重新同步 follower。
    auto leaderSceneObserver = [this](const QVariant&) -> QVariant {
        sync();
        return {};
    };

    m_posBinding = ReactiveBinding::createScenePosObserver(m_leader, leaderSceneObserver, this);

    // 监听 follower 的场景位置变化，用于捕获用户拖拽并更新偏移量。
    auto offsetObserver = [this](const QVariant&) -> QVariant {
        if (!m_leader || !m_follower || m_updating) {
            return {};
        }
        m_offset = m_follower->scenePos() - m_leader->scenePos();
        return {};
    };

    m_offsetBinding = ReactiveBinding::createScenePosObserver(m_follower, offsetObserver, this);

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

/// @brief 获取当前场景坐标相对偏移量。
QPointF FollowBinding::offset() const
{
    return m_offset;
}

/// @brief 设置新的场景坐标相对偏移量，并立即同步 follower 位置。
void FollowBinding::setOffset(const QPointF& offset)
{
    m_offset = offset;
    sync();
}

/// @brief 立即根据当前 leader 场景位置和偏移量重新同步 follower。
///
/// 计算目标场景位置 targetScenePos = leader->scenePos() + m_offset，
/// 然后将其转换到 follower 的父节点坐标系（若无父节点则直接使用场景坐标），
/// 最后写入 follower->pos()。
void FollowBinding::sync()
{
    if (!m_leader || !m_follower || m_updating) {
        return;
    }

    ReentrancyGuard guard(m_updating);
    const QPointF targetScenePos = m_leader->scenePos() + m_offset;

    QGraphicsItem* parent = m_follower->parentItem();
    const QPointF targetLocalPos = (parent != nullptr)
                                       ? parent->mapFromScene(targetScenePos)
                                       : targetScenePos;

    m_follower->setPos(targetLocalPos);
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
