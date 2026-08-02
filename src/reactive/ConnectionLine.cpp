/// @file ConnectionLine.cpp
/// @brief ConnectionLine 实现 —— 场景级连接线，通过场景位置 observer 监听两端并实时绘制。

#include <broaditem/reactive/ConnectionLine.h>
#include <broaditem/reactive/ReactiveBinding.h>
#include <broaditem/reactive/FollowBinding.h>
#include <broaditem/reactive/ReactiveProperty.h>

#include "ZOrder.h"

#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QPainter>
#include <QDebug>

namespace BroadItem {

// ══════════════════════════════════════════════════════════════════
// 静态工厂 create()
// ══════════════════════════════════════════════════════════════════

ConnectionLine* ConnectionLine::create(QGraphicsScene* scene,
                                       QObject* a,
                                       QObject* b,
                                       QObject* parent)
{
    if (!scene) {
        qWarning() << "ConnectionLine::create: scene is null";
        return nullptr;
    }
    if (!a) {
        qWarning() << "ConnectionLine::create: a is null";
        return nullptr;
    }
    if (!b) {
        qWarning() << "ConnectionLine::create: b is null";
        return nullptr;
    }
    if (a == b) {
        qWarning() << "ConnectionLine::create: a and b are the same object";
        return nullptr;
    }

    // 校验端点具备 pos 属性
    if (!a->property(Property::Pos.toLatin1().constData()).isValid()) {
        qWarning() << "ConnectionLine::create: a does not have pos property" << a;
        return nullptr;
    }
    if (!b->property(Property::Pos.toLatin1().constData()).isValid()) {
        qWarning() << "ConnectionLine::create: b does not have pos property" << b;
        return nullptr;
    }

    // 校验端点都在给定 scene 中
    auto* aObj = qobject_cast<QGraphicsObject*>(a);
    auto* bObj = qobject_cast<QGraphicsObject*>(b);
    if (!aObj || aObj->scene() != scene) {
        qWarning() << "ConnectionLine::create: a is not in the given scene" << a;
        return nullptr;
    }
    if (!bObj || bObj->scene() != scene) {
        qWarning() << "ConnectionLine::create: b is not in the given scene" << b;
        return nullptr;
    }

    auto* conn = new ConnectionLine(a, b, parent);
    scene->addItem(conn);

    // 计算初始 offset → 创建 FollowBinding（基于场景坐标，支持嵌套 parent）
    QPointF offset = bObj->scenePos() - aObj->scenePos();

    conn->m_followBinding = FollowBinding::create(a, b, offset, conn);
    if (!conn->m_followBinding) {
        qWarning() << "ConnectionLine::create: FollowBinding creation failed";
        scene->removeItem(conn);
        delete conn;
        return nullptr;
    }

    // 创建场景位置 observer（递归监听目标及其父链，只读不写）
    conn->m_aObserver = ReactiveBinding::createScenePosObserver(
        aObj,
        [conn](const QVariant&) -> QVariant {
            conn->onAPosChanged();
            return {};
        },
        conn);
    if (!conn->m_aObserver) {
        qWarning() << "ConnectionLine::create: a observer creation failed";
        scene->removeItem(conn);
        delete conn;
        return nullptr;
    }

    conn->m_bObserver = ReactiveBinding::createScenePosObserver(
        bObj,
        [conn](const QVariant&) -> QVariant {
            conn->onBPosChanged();
            return {};
        },
        conn);
    if (!conn->m_bObserver) {
        qWarning() << "ConnectionLine::create: b observer creation failed";
        scene->removeItem(conn);
        delete conn;
        return nullptr;
    }

    // 端点销毁 → 置空指针 + 隐藏线
    conn->m_aDestroyConnection = QObject::connect(
        a, &QObject::destroyed, conn, [conn]() {
            conn->m_a = nullptr;
            conn->setVisible(false);
        });
    conn->m_bDestroyConnection = QObject::connect(
        b, &QObject::destroyed, conn, [conn]() {
            conn->m_b = nullptr;
            conn->setVisible(false);
        });

    return conn;
}

// ══════════════════════════════════════════════════════════════════
// 构造 / 析构
// ══════════════════════════════════════════════════════════════════

ConnectionLine::ConnectionLine(QObject* a, QObject* b, QObject* parent)
    : QGraphicsObject(nullptr)
    , m_a(a)
    , m_b(b)
    , m_aObserver(nullptr)
    , m_bObserver(nullptr)
    , m_followBinding(nullptr)
    , m_pen(Qt::black, 1.0)
{
    if (parent) {
        QObject::setParent(parent);
    }

    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(ItemIsSelectable, false);
    // z 值层级约定见 ZOrder.h：连接线与装饰器外框同为 z=1
    setZValue(kDecorationZValue);

    auto* aObj = qobject_cast<QGraphicsObject*>(a);
    auto* bObj = qobject_cast<QGraphicsObject*>(b);
    m_lastAPos = aObj ? aObj->scenePos() : QPointF();
    m_lastBPos = bObj ? bObj->scenePos() : QPointF();
}

ConnectionLine::~ConnectionLine()
{
    disconnect(m_aDestroyConnection);
    disconnect(m_bDestroyConnection);

    // 显式 destroy() 再 delete：严格说并非必须——QObject 析构会自动断开
    // 以自身为 receiver 的连接，且这些 binding 以 this 为 QObject parent，
    // 本就会随父对象删除。此处提前断开是防御性写法，确保析构期间不再有
    // observer 回调访问正在销毁的本对象。
    if (m_followBinding) {
        m_followBinding->destroy();
        delete m_followBinding;
        m_followBinding = nullptr;
    }
    if (m_aObserver) {
        m_aObserver->destroy();
        delete m_aObserver;
        m_aObserver = nullptr;
    }
    if (m_bObserver) {
        m_bObserver->destroy();
        delete m_bObserver;
        m_bObserver = nullptr;
    }
}

// ══════════════════════════════════════════════════════════════════
// Getter / 样式
// ══════════════════════════════════════════════════════════════════

QObject* ConnectionLine::a() const { return m_a; }
QObject* ConnectionLine::b() const { return m_b; }
FollowBinding* ConnectionLine::followBinding() const { return m_followBinding; }
QPen ConnectionLine::pen() const { return m_pen; }

void ConnectionLine::setPen(const QPen& pen)
{
    if (m_pen == pen) return;
    m_pen = pen;
    update();
}

// ══════════════════════════════════════════════════════════════════
// QGraphicsItem 虚函数
// ══════════════════════════════════════════════════════════════════

QRectF ConnectionLine::boundingRect() const
{
    // adjusted(-1, -1, 1, 1)：画笔宽度补偿，确保描边线段的完整绘制区域被包围盒覆盖
    return QRectF(m_lastAPos, m_lastBPos).normalized().adjusted(-1, -1, 1, 1);
}

void ConnectionLine::paint(QPainter* painter,
                           const QStyleOptionGraphicsItem*,
                           QWidget*)
{
    painter->setPen(m_pen);
    painter->drawLine(m_lastAPos, m_lastBPos);
}

// ══════════════════════════════════════════════════════════════════
// 私有槽
// ══════════════════════════════════════════════════════════════════

void ConnectionLine::onAPosChanged()
{
    if (!m_a) return;

    auto* obj = qobject_cast<QGraphicsObject*>(m_a);
    if (!obj || !obj->scene()) {
        setVisible(false);
        return;
    }

    m_lastAPos = obj->scenePos();
    prepareGeometryChange();
    update();
}

void ConnectionLine::onBPosChanged()
{
    if (!m_b) return;

    auto* obj = qobject_cast<QGraphicsObject*>(m_b);
    if (!obj || !obj->scene()) {
        setVisible(false);
        return;
    }

    m_lastBPos = obj->scenePos();
    prepareGeometryChange();
    update();
}

} // namespace BroadItem
