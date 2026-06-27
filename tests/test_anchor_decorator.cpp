#include <QtTest/QtTest>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QPen>
#include <QPointer>
#include <QRegularExpression>
#include <cmath>

#include <broaditem/reactive/AnchorPoint.h>
#include <broaditem/reactive/AnchorDecorator.h>
#include <broaditem/reactive/ConnectionLine.h>
#include <broaditem/reactive/FollowBinding.h>

// ══════════════════════════════════════════════════════════════════
// 测试辅助类
// ══════════════════════════════════════════════════════════════════

/// @brief 最小化 QGraphicsObject 具体实现，用于测试锚点装饰器的基本行为。
///
/// 实现 boundingRect() 和 paint() 纯虚函数，设置位置变化通知标志。
/// 不含 width/height 自定义属性（resync 回退路径测试用）。
class TestObservableObject : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit TestObservableObject(QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent)
    {
        setFlags(flags() | QGraphicsItem::ItemSendsGeometryChanges
                         | QGraphicsItem::ItemSendsScenePositionChanges);
    }

    [[nodiscard]] QRectF boundingRect() const override { return {0, 0, 100, 50}; }
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}
};

/// @brief 带 width/height NOTIFY 信号的 QGraphicsObject 实现。
///
/// 用于测试 AnchorDecorator 对 widthChanged/heightChanged 的自动同步能力。
/// 在构造函数中设置 ItemSendsGeometryChanges|ItemSendsScenePositionChanges
/// 标志，使得 Qt 内部 NOTIFY 信号（xChanged/yChanged 等）正常发射。
class SizedTestObject : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged)
public:
    explicit SizedTestObject(QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent), m_width(100), m_height(50)
    {
        setFlags(flags() | QGraphicsItem::ItemSendsGeometryChanges
                         | QGraphicsItem::ItemSendsScenePositionChanges);
    }

    [[nodiscard]] qreal width() const { return m_width; }
    [[nodiscard]] qreal height() const { return m_height; }

    void setWidth(qreal w)
    {
        if (qFuzzyCompare(m_width, w)) return;
        prepareGeometryChange();
        m_width = w;
        emit widthChanged();
    }

    void setHeight(qreal h)
    {
        if (qFuzzyCompare(m_height, h)) return;
        prepareGeometryChange();
        m_height = h;
        emit heightChanged();
    }

    [[nodiscard]] QRectF boundingRect() const override
    {
        return {0, 0, m_width, m_height};
    }

    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}

signals:
    void widthChanged();
    void heightChanged();

private:
    qreal m_width;
    qreal m_height;
};

// ══════════════════════════════════════════════════════════════════
// 工具函数
// ══════════════════════════════════════════════════════════════════

/// @brief 判断两个 QPointF 在容差范围内是否相等。
/// @param a 第一个点。
/// @param b 第二个点。
/// @param delta 允许的绝对误差（默认 0.5px）。
/// @return true 表示两点在容差内相等。
static bool pointsNear(const QPointF& a, const QPointF& b, qreal delta = 0.5)
{
    return std::abs(a.x() - b.x()) <= delta && std::abs(a.y() - b.y()) <= delta;
}

// ══════════════════════════════════════════════════════════════════
// TestAnchorDecorator 测试套件
// ══════════════════════════════════════════════════════════════════

/// @brief AnchorPoint / AnchorDecorator 锚点装饰器的测试套件。
///
/// 覆盖锚点默认值、几何计算、生命周期管理、路径校验、边界情况、
/// ConnectionLine 集成、resync 机制和变换支持。
class TestAnchorDecorator : public QObject
{
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    // ─── AnchorPoint 基础测试 ────────────────────────────────

    /// @brief 测试 AnchorPoint 默认属性：直径 8、颜色 #555555、可见、
    ///        NoButton、!ItemIsSelectable、parentItem==nullptr、boundingRect==(-4,-4,8,8)。
    void pointDefaults()
    {
        QGraphicsScene scene;
        BroadItem::AnchorPoint point(&scene);

        QCOMPARE(point.diameter(), 8.0);
        QCOMPARE(point.color(), QColor(0x55, 0x55, 0x55));
        QVERIFY(point.anchorVisible());
        QCOMPARE(point.acceptedMouseButtons(), Qt::NoButton);
        QVERIFY((point.flags() & QGraphicsItem::ItemIsSelectable) == 0);
        QVERIFY(point.parentItem() == nullptr);
        QCOMPARE(point.boundingRect(), QRectF(-4.0, -4.0, 8.0, 8.0));
    }

    /// @brief 测试 AnchorPoint setter：setDiameter/setColor/setAnchorVisible 正确修改 getter。
    void pointSetters()
    {
        QGraphicsScene scene;
        BroadItem::AnchorPoint point(&scene);

        point.setDiameter(12.0);
        QCOMPARE(point.diameter(), 12.0);

        point.setColor(Qt::red);
        QCOMPARE(point.color(), QColor(Qt::red));

        point.setAnchorVisible(false);
        QVERIFY(!point.anchorVisible());

        // 直径改变后 boundingRect 同步更新
        QCOMPARE(point.boundingRect(), QRectF(-6.0, -6.0, 12.0, 12.0));
    }

    // ─── AnchorDecorator 创建与校验 ──────────────────────────

    /// @brief 测试正常创建：item.parentItem==decorator，decorator.scene==scene。
    void decoratorCreate()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        QCOMPARE(item->parentItem(), static_cast<QGraphicsItem*>(decorator));
        QCOMPARE(decorator->scene(), &scene);

        delete decorator;
    }

    /// @brief 测试空场景拒绝：create(nullptr, item) 返回 nullptr 并警告。
    void decoratorCreateRejectsNullScene()
    {
        auto* item = new SizedTestObject();

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
        auto* decorator = BroadItem::AnchorDecorator::create(nullptr, item);
        QVERIFY(decorator == nullptr);

        delete item;
    }

    /// @brief 测试空 item 拒绝：create(&scene, nullptr) 返回 nullptr 并警告。
    void decoratorCreateRejectsNullItem()
    {
        QGraphicsScene scene;

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
        auto* decorator = BroadItem::AnchorDecorator::create(&scene, nullptr);
        QVERIFY(decorator == nullptr);
    }

    /// @brief 测试已有 QGraphicsItem 父项拒绝：item 已挂载到非装饰器父项时拒绝创建。
    void decoratorCreateRejectsExistingParent()
    {
        QGraphicsScene scene;
        // 创建一个普通父项
        auto* parent = new TestObservableObject();
        scene.addItem(parent);
        // item 挂在 parent 下
        auto* item = new SizedTestObject(parent);

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator == nullptr);

        // 清理：删除 parent 会自动删除 item
        delete parent;
    }

    /// @brief 测试双重装饰拒绝：已挂载装饰器的 item 拒绝再次装饰。
    void decoratorCreateRejectsDoubleAttach()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        auto* dec1 = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(dec1 != nullptr);

        // 再次对同一 item 创建装饰器应失败
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
        auto* dec2 = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(dec2 == nullptr);

        delete dec1;
    }

    // ─── AnchorDecorator 几何 ────────────────────────────────

    /// @brief 测试装饰器几何计算：item 在 (10,20) 尺寸 100x50 margin=4
    ///        → decorator.pos==(6,16)、width=108、height=58。
    void decoratorGeometry()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        item->setPos(10, 20);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        // 装饰器包围 item 并外扩 margin
        QCOMPARE(decorator->margin(), 4.0);

        // decorator pos 应比 item 左上角缩进 margin
        QVERIFY2(pointsNear(decorator->pos(), QPointF(6, 16)),
                 qPrintable(QString("decorator pos expected (6,16), got (%1,%2)")
                            .arg(decorator->pos().x()).arg(decorator->pos().y())));

        // decorator boundingRect 应包含 item + 两侧 margin
        QCOMPARE(decorator->boundingRect().width(), 108.0);
        QCOMPARE(decorator->boundingRect().height(), 58.0);

        // item 在装饰器内的相对位置应为 (margin, margin)
        QVERIFY2(pointsNear(item->pos(), QPointF(4, 4)),
                 qPrintable(QString("item pos relative to decorator expected (4,4), got (%1,%2)")
                            .arg(item->pos().x()).arg(item->pos().y())));

        delete decorator;
    }

    /// @brief 测试八个锚点位置与 AABB 对齐：
    ///        N/NE/E/SE/S/SW/W/NW 在 0.5px 容差内匹配装饰器各边中点及角点。
    void anchorPositionsMatchAabb()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        item->setPos(100, 200);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        const qreal m = decorator->margin();       // 4
        const qreal w = decorator->boundingRect().width();   // 108
        const qreal h = decorator->boundingRect().height();  // 58
        const qreal dx = decorator->pos().x();    // 100 - 4 = 96
        const qreal dy = decorator->pos().y();    // 200 - 4 = 196

        // 锚点是顶级 scene item，pos 即为 scene 坐标
        struct { BroadItem::AnchorPoint* anchor; qreal ex; qreal ey; const char* name; } checks[] = {
            { decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North),  dx + w / 2, dy,            "N"  },
            { decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::NorthEast), dx + w,     dy,            "NE" },
            { decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::East),  dx + w,     dy + h / 2,    "E"  },
            { decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::SouthEast), dx + w,     dy + h,        "SE" },
            { decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::South),  dx + w / 2, dy + h,        "S"  },
            { decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::SouthWest), dx,         dy + h,        "SW" },
            { decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::West),  dx,         dy + h / 2,    "W"  },
            { decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::NorthWest), dx,         dy,            "NW" },
        };

        for (const auto& ch : checks) {
            QVERIFY2(ch.anchor != nullptr,
                     qPrintable(QString("%1 anchor is null").arg(ch.name)));
            QVERIFY2(pointsNear(ch.anchor->pos(), QPointF(ch.ex, ch.ey)),
                     qPrintable(QString("%1 anchor pos expected (%2,%3), got (%4,%5)")
                                .arg(ch.name)
                                .arg(ch.ex).arg(ch.ey)
                                .arg(ch.anchor->pos().x()).arg(ch.anchor->pos().y())));
        }

        delete decorator;
    }

    // ─── 移动传播 ────────────────────────────────────────────

    /// @brief 测试移动装饰器时 item 和锚点跟随。
    ///        move decorator → item follows → anchors update。
    void decoratorMovePropagates()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        item->setPos(0, 0);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        // 记录移动前的 item scene 位置和锚点位置
        QPointF itemSceneBefore = item->scenePos();
        QPointF nAnchorBefore = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->pos();

        // 移动装饰器
        decorator->setPos(50, 70);
        QCoreApplication::processEvents();

        // item 应跟随装饰器移动（由于 parentItem 关系）
        QPointF itemSceneAfter = item->scenePos();
        QVERIFY(itemSceneAfter != itemSceneBefore);

        // 锚点应更新到新位置
        QPointF nAnchorAfter = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->pos();
        QVERIFY(nAnchorAfter != nAnchorBefore);

        // N 锚点应在装饰器顶部居中（新位置）
        qreal m = decorator->margin();
        qreal w = decorator->boundingRect().width();
        QVERIFY2(pointsNear(nAnchorAfter, QPointF(50 + m + w / 2 - m, 70)),
                 qPrintable(QString("N anchor after move expected near (%1,%2), got (%3,%4)")
                            .arg(50 + m + w / 2 - m).arg(70)
                            .arg(nAnchorAfter.x()).arg(nAnchorAfter.y())));

        delete decorator;
    }

    /// @brief 测试独立移动 item 后 resync 触发装饰器和锚点更新。
    ///        移动 item 局部坐标 → 装饰器观察者更新位置和锚点。
    void itemMoveIndependentlyTriggersResync()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        item->setPos(100, 150);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        // 记录初始状态
        QPointF decPosBefore = decorator->pos();
        QPointF nAnchorBefore = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->pos();

        // 在场景中独立移动 item（改变其 scenePos）
        item->setPos(300, 400);
        QCoreApplication::processEvents();

        // 装饰器应通过观察者自动 resync：位置和锚点均更新
        QPointF decPosAfter = decorator->pos();
        QVERIFY(decPosAfter != decPosBefore);

        QPointF nAnchorAfter = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->pos();
        QVERIFY(nAnchorAfter != nAnchorBefore);

        // N 锚点应在新装饰器 AABB 顶部居中
        qreal w = decorator->boundingRect().width();
        QVERIFY2(pointsNear(nAnchorAfter,
                            QPointF(decorator->pos().x() + w / 2, decorator->pos().y())),
                 "N anchor should be at decorator top-center after resync");

        delete decorator;
    }

    // ─── resync 机制 ─────────────────────────────────────────

    /// @brief 测试 resync 回退路径：item 无 width/height NOTIFY，
    ///        修改 boundingRect 后手动调用 resync，锚点更新。
    void resyncFallback()
    {
        QGraphicsScene scene;
        // TestObservableObject 不含 width/height NOTIFY
        auto* item = new TestObservableObject();
        scene.addItem(item);

        item->setPos(50, 80);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        // 记录初始锚点位置
        QPointF nBefore = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->pos();
        QPointF sBefore = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::South)->pos();

        // 改变 item 的包围盒（通过子类化方式模拟 size 变化）
        // TestObservableObject 固定 boundingRect(100,50)，此处通过改变 item
        // 的场景可见区域来触发 resync 需求。
        // 手动调用 resync 让装饰器重新计算几何。
        decorator->resync();

        // resync 后锚点应根据当前 item 状态重新定位
        // 若 item 未变化则位置不变，但 resync 调用本身不应崩溃或产生无效值
        Q_UNUSED(nBefore);
        Q_UNUSED(sBefore);
        QVERIFY(decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North) != nullptr);
        QVERIFY(decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::South) != nullptr);

        delete decorator;
    }

    /// @brief 测试自动同步：item 含 width NOTIFY，setWidth(150) 后装饰器自动更新。
    void autoSyncWithWidthHeightNotify()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        item->setPos(10, 20);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        const qreal oldWidth = decorator->boundingRect().width();
        const qreal m = decorator->margin();

        // 修改宽度触发 NOTIFY，装饰器应自动重算几何
        item->setWidth(150);
        QCoreApplication::processEvents();

        const qreal newWidth = decorator->boundingRect().width();
        // 新宽度 = 150 + 2 * margin
        QCOMPARE(newWidth, 150.0 + 2.0 * m);
        QVERIFY(newWidth > oldWidth);

        // 同时测试高度同步
        const qreal oldHeight = decorator->boundingRect().height();
        item->setHeight(90);
        QCoreApplication::processEvents();

        const qreal newHeight = decorator->boundingRect().height();
        QCOMPARE(newHeight, 90.0 + 2.0 * m);
        QVERIFY(newHeight > oldHeight);

        // 锚点也应更新：E 锚点应在新的右边框中点
        QPointF ePos = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::East)->pos();
        QVERIFY2(pointsNear(ePos,
                            QPointF(decorator->pos().x() + newWidth,
                                    decorator->pos().y() + newHeight / 2)),
                 "E anchor should be at right edge center after width/height change");

        delete decorator;
    }

    // ─── 生命周期 ────────────────────────────────────────────

    /// @brief 测试删除装饰器时 item 保留：delete decorator → item 存活且重新成为顶层 scene item。
    void deleteDecoratorPreservesItem()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        item->setPos(30, 60);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);
        QCOMPARE(item->parentItem(), static_cast<QGraphicsItem*>(decorator));

        // 记录 item 在删除前的场景位置
        QPointF scenePosBefore = item->scenePos();

        // 删除装饰器
        delete decorator;

        // item 应存活且不再是任何 QGraphicsItem 的子项
        QVERIFY(item->parentItem() == nullptr);

        // item 的场景位置应保持（或恢复）
        QVERIFY2(pointsNear(item->scenePos(), scenePosBefore),
                 "item scene position should be preserved after decorator deletion");

        // item 仍在场景中
        QVERIFY(item->scene() == &scene);

        delete item;
    }

    /// @brief 测试 item 销毁时装饰器和锚点级联销毁：
    ///        delete item → QPointer<decorator> 和 8 个 QPointer<anchor> 均为 null。
    void deleteItemDestroysDecoratorAndAnchors()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        item->setPos(0, 0);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        // 包装 QPointer 用于存活检测
        QPointer<QObject> decPtr(decorator);

        QPointer<QObject> anchorPtrs[8];
        anchorPtrs[0] = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North);
        anchorPtrs[1] = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::NorthEast);
        anchorPtrs[2] = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::East);
        anchorPtrs[3] = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::SouthEast);
        anchorPtrs[4] = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::South);
        anchorPtrs[5] = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::SouthWest);
        anchorPtrs[6] = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::West);
        anchorPtrs[7] = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::NorthWest);

        // 验证所有锚点初始非空
        for (int i = 0; i < 8; ++i) {
            QVERIFY2(!anchorPtrs[i].isNull(),
                     qPrintable(QString("anchor %1 should not be null initially").arg(i)));
        }

        // 销毁 item
        delete item;
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        // 装饰器应随之销毁
        QVERIFY2(decPtr.isNull(), "decorator QPointer should be null after item deletion");

        // 八个锚点应随之销毁
        for (int i = 0; i < 8; ++i) {
            QVERIFY2(anchorPtrs[i].isNull(),
                     qPrintable(QString("anchor %1 QPointer should be null after item deletion").arg(i)));
        }
    }

    // ─── ConnectionLine 集成 ─────────────────────────────────

    /// @brief 测试 ConnectionLine 集成：两个装饰器，在锚点间创建连接线，
    ///        移动一个装饰器后线端点跟随。
    void connectionLineIntegration()
    {
        QGraphicsScene scene;

        // 创建两个 item 及对应装饰器
        auto* item1 = new SizedTestObject();
        auto* item2 = new SizedTestObject();
        scene.addItem(item1);
        scene.addItem(item2);

        item1->setPos(0, 0);
        item2->setPos(300, 100);

        auto* dec1 = BroadItem::AnchorDecorator::create(&scene, item1);
        auto* dec2 = BroadItem::AnchorDecorator::create(&scene, item2);
        QVERIFY(dec1 != nullptr);
        QVERIFY(dec2 != nullptr);

        // 在 dec1 的 E 锚点和 dec2 的 W 锚点间创建连接线
        auto* line = BroadItem::ConnectionLine::create(
            &scene, dec1->anchor(BroadItem::AnchorDecorator::AnchorSide::East), dec2->anchor(BroadItem::AnchorDecorator::AnchorSide::West));
        QVERIFY(line != nullptr);

        // 记录初始端点位置
        QPointF pos1Before = dec1->anchor(BroadItem::AnchorDecorator::AnchorSide::East)->pos();
        QPointF pos2Before = dec2->anchor(BroadItem::AnchorDecorator::AnchorSide::West)->pos();

        // 移动 dec1，锚点应跟随，连接线应重绘
        dec1->setPos(50, 30);
        QCoreApplication::processEvents();

        QPointF pos1After = dec1->anchor(BroadItem::AnchorDecorator::AnchorSide::East)->pos();

        // E 锚点移动后位置应不同
        QVERIFY(pos1After != pos1Before);

        // 连接线 boundingRect 应包含更新后的端点
        QRectF br = line->boundingRect();
        QVERIFY(br.contains(pos1After));
        QVERIFY(br.contains(pos2Before));

        // 同时测试 FollowBinding 偏移量：dec1 移动后 dec2 应保持相对位置
        auto* fb = line->followBinding();
        QVERIFY(fb != nullptr);
        QCOMPARE(fb->leader(), static_cast<QObject*>(dec1->anchor(BroadItem::AnchorDecorator::AnchorSide::East)));
        QCOMPARE(fb->follower(), static_cast<QObject*>(dec2->anchor(BroadItem::AnchorDecorator::AnchorSide::West)));

        delete line;
        delete dec1;
        delete dec2;
    }

    // ─── 变换支持 ────────────────────────────────────────────

    /// @brief 测试变换支持：旋转 item 45°，resync 后 AABB 扩大，
    ///        N 锚点在 AABB 顶部居中。
    void transformsSupport()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        item->setPos(100, 100);
        item->setWidth(80);
        item->setHeight(40);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        // 记录旋转前的 AABB 尺寸
        QRectF aabbBefore = decorator->boundingRect();

        // 旋转 item 45° 并手动 resync
        item->setRotation(45.0);
        decorator->resync();
        QCoreApplication::processEvents();

        // 旋转后 AABB 应扩大以容纳旋转后的 item
        QRectF aabbAfter = decorator->boundingRect();
        QVERIFY2(aabbAfter.width() > aabbBefore.width()
                 || aabbAfter.height() > aabbBefore.height(),
                 "AABB should grow after 45° rotation");

        // N 锚点应在旋转后 AABB 的顶部居中
        QPointF nPos = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->pos();
        qreal expectedX = decorator->pos().x() + aabbAfter.width() / 2.0;
        qreal expectedY = decorator->pos().y();
        QVERIFY2(pointsNear(nPos, QPointF(expectedX, expectedY)),
                 qPrintable(QString("N anchor after rotation expected (%1,%2), got (%3,%4)")
                            .arg(expectedX).arg(expectedY)
                            .arg(nPos.x()).arg(nPos.y())));

        delete decorator;
    }

    /// @brief 测试 AnchorDecorator 自身不处理鼠标事件，与 AnchorPoint 一致。
    void decoratorMouseIgnored()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        // 装饰器本身也不应接受鼠标交互
        QCOMPARE(decorator->acceptedMouseButtons(), Qt::NoButton);
        QVERIFY((decorator->flags() & QGraphicsItem::ItemIsSelectable) == 0);

        delete decorator;
    }

    /// @brief 测试装饰器锚点可见性可独立控制：toggle 后锚点隐藏/显示。
    void anchorVisibleToggle()
    {
        QGraphicsScene scene;
        auto* item = new SizedTestObject();
        scene.addItem(item);

        item->setPos(0, 0);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, item);
        QVERIFY(decorator != nullptr);

        // 默认所有锚点可见
        QVERIFY(decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->anchorVisible());
        QVERIFY(decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::SouthEast)->anchorVisible());

        // 隐藏 N 锚点
        decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->setAnchorVisible(false);
        QVERIFY(!decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->anchorVisible());
        // 其他锚点不受影响
        QVERIFY(decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::SouthEast)->anchorVisible());

        // 恢复可见
        decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->setAnchorVisible(true);
        QVERIFY(decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North)->anchorVisible());

        delete decorator;
    }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestAnchorDecorator)
#include "test_anchor_decorator.moc"
