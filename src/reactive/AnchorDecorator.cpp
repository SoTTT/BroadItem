/// @file AnchorDecorator.cpp
/// @brief AnchorDecorator 实现 —— 装饰器图元，为 QGraphicsObject 添加锚点和外框。

#include <broaditem/reactive/AnchorDecorator.h>
#include <broaditem/reactive/AnchorPoint.h>
#include <broaditem/reactive/ReactiveBinding.h>
#include <broaditem/reactive/ReactiveProperty.h>

#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QMetaProperty>
#include <QPainter>
#include <QDebug>

namespace BroadItem {

// ══════════════════════════════════════════════════════════════════
// 静态辅助函数
// ══════════════════════════════════════════════════════════════════

/// @brief 检查 QObject 的指定属性是否有 NOTIFY 信号。
///
/// 用于预检 decorated 的 width/height 属性是否支持响应式监听。
/// ReactiveBinding::isValidProperty 为 private，不可直接调用，
/// 因此通过 QMetaObject 的 indexOfProperty + hasNotifySignal 自建预检逻辑，
/// 避免在属性无效时调用 createObserver 产生 qWarning。
/// @param obj 要检查的 QObject。
/// @param propName 属性名。
/// @return true 表示属性存在且有 NOTIFY 信号。
bool AnchorDecorator::hasNotifyProperty(const QObject* obj, const QString& propName)
{
    if (!obj) return false;
    const QMetaObject* meta = obj->metaObject();
    int idx = meta->indexOfProperty(propName.toLatin1().constData());
    if (idx < 0) return false;
    return meta->property(idx).hasNotifySignal();
}

// ══════════════════════════════════════════════════════════════════
// 静态工厂 create()
// ══════════════════════════════════════════════════════════════════

AnchorDecorator* AnchorDecorator::create(QGraphicsScene* scene,
                                         QGraphicsObject* decorated,
                                         QObject* parent)
{
    // 校验 scene 非空
    if (!scene) {
        qWarning() << "AnchorDecorator::create: scene is null";
        return nullptr;
    }

    // 校验 decorated 非空
    if (!decorated) {
        qWarning() << "AnchorDecorator::create: decorated is null";
        return nullptr;
    }

    // 校验 decorated 无已有 parentItem（尚未被其他图元父化）
    if (decorated->parentItem()) {
        qWarning() << "AnchorDecorator::create: decorated already has a parentItem";
        return nullptr;
    }

    auto* decorator = new AnchorDecorator(scene, decorated, parent);
    scene->addItem(decorator);

    // 将 decorated 设为 decorator 的 QGraphicsItem 子节点
    // 注意：仅 QGraphicsItem 父子关系，不涉及 QObject 所有权
    decorated->setParentItem(decorator);
    decorator->setZValue(1);

    // 创建 pos 观察者——始终连接，因 pos 属性的 NOTIFY 信号天然存在
    decorator->m_posObserver = ReactiveBinding::createObserver(
        decorated, Property::Pos,
        [decorator](const QVariant&) -> QVariant {
            // 避免在 ReactiveBinding::evaluate 栈内同步调用 computeAndApplyGeometry，
            // 否则调整 decorated pos 会触发同一 observer 重入，被判定为 cycle。
            if (!decorator->m_inGeometryUpdate) {
                QMetaObject::invokeMethod(decorator, "resync", Qt::QueuedConnection);
            }
            return {};
        },
        decorator);

    if (!decorator->m_posObserver) {
        qWarning() << "AnchorDecorator::create: pos observer creation failed";
        scene->removeItem(decorator);
        delete decorator;
        return nullptr;
    }

    // 创建 width 观察者——仅当 decorated 有 width 属性且带 NOTIFY 信号时
    // 预检避免 createObserver 因属性无效而输出 qWarning
    if (hasNotifyProperty(decorated, QStringLiteral("width"))) {
        decorator->m_widthObserver = ReactiveBinding::createObserver(
            decorated, QStringLiteral("width"),
            [decorator](const QVariant&) -> QVariant {
                if (!decorator->m_inGeometryUpdate) {
                    QMetaObject::invokeMethod(decorator, "resync", Qt::QueuedConnection);
                }
                return {};
            },
            decorator);
        // width observer 创建失败不是致命错误，仅记录日志
        if (!decorator->m_widthObserver) {
            qWarning() << "AnchorDecorator::create: width observer creation failed";
        }
    }

    // 创建 height 观察者——仅当 decorated 有 height 属性且带 NOTIFY 信号时
    if (hasNotifyProperty(decorated, QStringLiteral("height"))) {
        decorator->m_heightObserver = ReactiveBinding::createObserver(
            decorated, QStringLiteral("height"),
            [decorator](const QVariant&) -> QVariant {
                if (!decorator->m_inGeometryUpdate) {
                    QMetaObject::invokeMethod(decorator, "resync", Qt::QueuedConnection);
                }
                return {};
            },
            decorator);
        // height observer 创建失败不是致命错误，仅记录日志
        if (!decorator->m_heightObserver) {
            qWarning() << "AnchorDecorator::create: height observer creation failed";
        }
    }

    // 连接装饰器自身位置变化：装饰器被拖动或 setPos 时，锚点应跟随更新
    QObject::connect(decorator, &QGraphicsObject::xChanged, decorator, [decorator]() {
        decorator->updateAnchorPositions();
    });
    QObject::connect(decorator, &QGraphicsObject::yChanged, decorator, [decorator]() {
        decorator->updateAnchorPositions();
    });

    // 监听 decorated 销毁：置空指针 + deleteLater 自我调度删除
    // 不在槽中直接 delete this，避免在信号发射过程中删除对象
    decorator->m_decoratedDestroyConnection = QObject::connect(
        decorated, &QObject::destroyed, decorator, [decorator]() {
            decorator->m_decorated = nullptr;
            decorator->deleteLater();
        });

    // 初始几何计算：根据 decorated 的当前包围盒设定装饰器位置和锚点
    decorator->computeAndApplyGeometry();

    return decorator;
}

// ══════════════════════════════════════════════════════════════════
// 构造 / 析构
// ══════════════════════════════════════════════════════════════════

AnchorDecorator::AnchorDecorator(QGraphicsScene* scene,
                                 QGraphicsObject* decorated,
                                 QObject* parent)
    : QGraphicsObject(nullptr)          // QGraphicsItem parent 始终为 nullptr（顶级场景项）
    , m_decorated(decorated)
    , m_anchors{}                        // 零初始化指针数组
    , m_pen(Qt::black, 1.0)
    , m_margin(4.0)                     // 默认边距 4px，与计划一致
    , m_width(0)
    , m_height(0)
    , m_anchorVisible(true)
    , m_inGeometryUpdate(false)
    , m_posObserver(nullptr)
    , m_widthObserver(nullptr)
    , m_heightObserver(nullptr)
{
    if (parent) {
        QObject::setParent(parent);
    }

    // 禁用鼠标交互：装饰器不处理点击或选择
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(ItemIsSelectable, false);

    // 创建 8 个 AnchorPoint：均为顶级场景图元，QObject parent = this
    // AnchorPoint 构造签名为 AnchorPoint(QGraphicsScene*, QObject*)
    // 锚点通过 QObject 父子关系随装饰器析构自动销毁
    for (int i = 0; i < 8; ++i) {
        m_anchors[i] = new AnchorPoint(scene, this);
    }
}

AnchorDecorator::~AnchorDecorator()
{
    // 断开 decorated 销毁信号监听
    disconnect(m_decoratedDestroyConnection);

    // 如果 decorated 仍存活，将其还原为场景顶级图元
    // 保持场景坐标不变，使 decorated 在装饰器移除后位置不变
    if (m_decorated) {
        QPointF pos = m_decorated->scenePos();
        m_decorated->setParentItem(nullptr);
        m_decorated->setPos(pos);
        m_decorated = nullptr;
    }

    // 显式删除 8 个 AnchorPoint
    // AnchorPoint 的 QObject parent 已设为 this，delete 将递归释放
    for (int i = 0; i < 8; ++i) {
        if (m_anchors[i]) {
            delete m_anchors[i];
            m_anchors[i] = nullptr;
        }
    }

    // 清理 ReactiveBinding observer：先 destroy 断开信号，再 delete 释放内存
    // 参考 ConnectionLine 析构中的清理模式
    if (m_posObserver) {
        m_posObserver->destroy();
        delete m_posObserver;
        m_posObserver = nullptr;
    }
    if (m_widthObserver) {
        m_widthObserver->destroy();
        delete m_widthObserver;
        m_widthObserver = nullptr;
    }
    if (m_heightObserver) {
        m_heightObserver->destroy();
        delete m_heightObserver;
        m_heightObserver = nullptr;
    }
}

// ══════════════════════════════════════════════════════════════════
// Getter / 样式
// ══════════════════════════════════════════════════════════════════

AnchorPoint* AnchorDecorator::anchor(AnchorSide side) const
{
    int idx = static_cast<int>(side);
    if (idx >= 0 && idx < 8) {
        return m_anchors[idx];
    }
    return nullptr;
}

QPen AnchorDecorator::pen() const { return m_pen; }

void AnchorDecorator::setPen(const QPen& pen)
{
    if (m_pen == pen) return;
    m_pen = pen;
    update();
}

qreal AnchorDecorator::margin() const { return m_margin; }

void AnchorDecorator::setMargin(qreal margin)
{
    if (qFuzzyCompare(m_margin, margin)) return;
    m_margin = margin;
    computeAndApplyGeometry();
}

void AnchorDecorator::setAnchorVisible(bool visible)
{
    if (m_anchorVisible == visible) return;
    m_anchorVisible = visible;
    for (int i = 0; i < 8; ++i) {
        if (m_anchors[i]) {
            m_anchors[i]->setAnchorVisible(visible);
        }
    }
}

bool AnchorDecorator::anchorVisible() const { return m_anchorVisible; }

qreal AnchorDecorator::width() const { return m_width; }

qreal AnchorDecorator::height() const { return m_height; }

// ══════════════════════════════════════════════════════════════════
// QGraphicsItem 虚函数
// ══════════════════════════════════════════════════════════════════

QRectF AnchorDecorator::boundingRect() const
{
    return QRectF(0, 0, m_width, m_height);
}

void AnchorDecorator::paint(QPainter* painter,
                            const QStyleOptionGraphicsItem*,
                            QWidget*)
{
    painter->setPen(m_pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(boundingRect());
}

// ══════════════════════════════════════════════════════════════════
// 几何体计算
// ══════════════════════════════════════════════════════════════════

void AnchorDecorator::computeAndApplyGeometry()
{
    // 防止 observer 回调触发递归：本函数内部会调整 decorated pos
    Q_ASSERT(!m_inGeometryUpdate);
    m_inGeometryUpdate = true;

    // 步骤 1：守卫检查——decorated 已销毁则跳过
    if (!m_decorated) {
        m_inGeometryUpdate = false;
        return;
    }

    // 步骤 2：获取 decorated 在场景中的轴对齐包围盒（AABB）
    // sceneBoundingRect() 考虑了图元自身的变换（缩放、旋转等），
    // 但返回的是轴对齐矩形，保证装饰器外框始终为轴对齐
    QRectF sceneAabb = m_decorated->sceneBoundingRect();

    // 步骤 3：设置装饰器自身在场景中的位置
    // 装饰器左上角 = sceneAabb 左上角向外扩展 margin 像素
    setPos(sceneAabb.topLeft() - QPointF(m_margin, m_margin));

    // 步骤 4：调整 decorated 在装饰器内的局部坐标
    // itemBRect 是 decorated 自身坐标系下的 boundingRect（可能原点非零）
    // 减去 itemBRect.x()/y() 以补偿 decorated 自身包围盒的偏移
    QRectF itemBRect = m_decorated->boundingRect();
    m_decorated->setPos(QPointF(m_margin - itemBRect.x(), m_margin - itemBRect.y()));

    // 步骤 5：更新 m_width/m_height，仅在值变化时发射信号
    qreal newWidth = sceneAabb.width() + 2 * m_margin;
    qreal newHeight = sceneAabb.height() + 2 * m_margin;

    if (!qFuzzyCompare(newWidth, m_width)) {
        m_width = newWidth;
        emit widthChanged();
    }
    if (!qFuzzyCompare(newHeight, m_height)) {
        m_height = newHeight;
        emit heightChanged();
    }

    // 步骤 6：根据新的宽高重新计算 8 个锚点位置
    updateAnchorPositions();

    // 步骤 7：通知 QGraphicsView 几何体已变更，触发重绘
    prepareGeometryChange();
    update();

    m_inGeometryUpdate = false;
}

void AnchorDecorator::updateAnchorPositions()
{
    // 8 个锚点位置（场景坐标系中）
    // 锚点是顶级场景图元，其 QGraphicsItem parent = nullptr，
    // 因此 setPos 的参数必须是 scene 坐标，而非 decorator 局部坐标。
    QPointF base = scenePos();
    const qreal w = m_width;
    const qreal h = m_height;
    const qreal hw = w / 2.0;
    const qreal hh = h / 2.0;

    QPointF positions[8] = {
        base + QPointF(hw, 0),    // North —— 上边中点
        base + QPointF(w, 0),     // NorthEast —— 右上角
        base + QPointF(w, hh),    // East —— 右边中点
        base + QPointF(w, h),     // SouthEast —— 右下角
        base + QPointF(hw, h),    // South —— 下边中点
        base + QPointF(0, h),     // SouthWest —— 左下角
        base + QPointF(0, hh),    // West —— 左边中点
        base + QPointF(0, 0)      // NorthWest —— 左上角
    };

    for (int i = 0; i < 8; ++i) {
        if (m_anchors[i]) {
            m_anchors[i]->setPos(positions[i]);
        }
    }
}

// ══════════════════════════════════════════════════════════════════
// 公有槽
// ══════════════════════════════════════════════════════════════════

void AnchorDecorator::resync()
{
    // 强制重新同步：当 decorated 的变换（缩放/旋转等）发生变化后，
    // sceneBoundingRect 会改变，需要重新计算装饰器的轴对齐包围盒
    computeAndApplyGeometry();
}

} // namespace BroadItem
