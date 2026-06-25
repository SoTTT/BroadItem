#include <QtTest/QtTest>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QPen>

#include <broaditem/reactive/ConnectionLine.h>
#include <broaditem/reactive/FollowBinding.h>

/// @brief 最小化 QGraphicsObject 具体实现，用于测试 ConnectionLine 逻辑。
///
/// 实现 QGraphicsObject 要求的 boundingRect() 和 paint() 纯虚函数，
/// 在构造函数中设置 ItemSendsGeometryChanges|ItemSendsScenePositionChanges
/// 标志，使得 Qt 内部 NOTIFY 信号（xChanged/yChanged 等）正常发射。
class TestObservableObject : public QGraphicsObject {
    Q_OBJECT
public:
    explicit TestObservableObject(QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent)
    {
        setFlags(flags() | QGraphicsItem::ItemSendsGeometryChanges
                         | QGraphicsItem::ItemSendsScenePositionChanges);
    }

    [[nodiscard]] QRectF boundingRect() const override { return {0, 0, 100, 100}; }
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}
};

/// @brief ConnectionLine 连接线 item 的测试套件。
///
/// 覆盖 create 校验、几何跟随、FollowBinding 配对、生命周期管理、
/// 端点销毁行为和样式/zOrder/鼠标事件设定。
class TestConnectionLine : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    /// @brief 测试正常创建：两个 item 加入 scene，create 返回非 null。
    void createValid()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        TestObservableObject b;
        scene.addItem(&a);
        scene.addItem(&b);

        a.setPos(0, 0);
        b.setPos(100, 0);

        auto* line = BroadItem::ConnectionLine::create(&scene, &a, &b);
        QVERIFY(line != nullptr);

        delete line;
    }

    /// @brief 测试空指针输入：create 传入 nullptr 返回 null 并触发 qWarning。
    void createNullInputs()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        scene.addItem(&a);

        // a 为 nullptr
        {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
            auto* line = BroadItem::ConnectionLine::create(&scene, nullptr, &a);
            QVERIFY(line == nullptr);
        }

        // b 为 nullptr
        {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
            auto* line = BroadItem::ConnectionLine::create(&scene, &a, nullptr);
            QVERIFY(line == nullptr);
        }
    }

    /// @brief 测试自连接：create(scene, &a, &a) 返回 null。
    void createSelfConnection()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        scene.addItem(&a);

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
        auto* line = BroadItem::ConnectionLine::create(&scene, &a, &a);
        QVERIFY(line == nullptr);
    }

    /// @brief 测试创建后 followBinding 非 null，a()/b() 返回正确的端点。
    void followBindingCreated()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        TestObservableObject b;
        scene.addItem(&a);
        scene.addItem(&b);

        a.setPos(0, 0);
        b.setPos(100, 0);

        auto* line = BroadItem::ConnectionLine::create(&scene, &a, &b);
        QVERIFY(line != nullptr);

        QVERIFY(line->followBinding() != nullptr);
        QCOMPARE(line->a(), static_cast<QObject*>(&a));
        QCOMPARE(line->b(), static_cast<QObject*>(&b));

        delete line;
    }

    /// @brief 测试几何跟随：移动端点后 boundingRect 包含新位置。
    void geometryFollowsPos()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        TestObservableObject b;
        scene.addItem(&a);
        scene.addItem(&b);

        a.setPos(0, 0);
        b.setPos(100, 0);

        auto* line = BroadItem::ConnectionLine::create(&scene, &a, &b);
        QVERIFY(line != nullptr);

        // 移动 a 到 (50, 50)，FollowBinding 会将 b 同步移动（offset = 100,0）
        a.setPos(50, 50);
        QCoreApplication::processEvents();

        QRectF br = line->boundingRect();
        // boundingRect 应包含 a 的新位置和 b 的跟随位置
        QVERIFY(br.contains(QPointF(50, 50)));
        QVERIFY(br.contains(b.pos()));

        // 再移动 b 到 (200, 200)，验证单独移动 b 也反映在 boundingRect
        b.setPos(200, 200);
        QCoreApplication::processEvents();

        br = line->boundingRect();
        QVERIFY(br.contains(a.pos()));
        QVERIFY(br.contains(QPointF(200, 200)));

        delete line;
    }

    /// @brief 测试 FollowBinding 配对：移动 a 后 b 跟随（offset 保持）。
    void followCouplingWorks()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        TestObservableObject b;
        scene.addItem(&a);
        scene.addItem(&b);

        a.setPos(10, 20);
        b.setPos(110, 120);
        // b - a = (100, 100)

        auto* line = BroadItem::ConnectionLine::create(&scene, &a, &b);
        QVERIFY(line != nullptr);

        // 移动 a 到 (50, 70)，b 应跟随到 (150, 170)
        a.setPos(50, 70);
        QCoreApplication::processEvents();

        // FollowBinding 通过 pos 绑定同步，b 应保持 offset
        QCOMPARE(b.pos(), QPointF(150, 170));

        delete line;
    }

    /// @brief 测试析构后绑定被销毁：delete line 后再移动 a，b 不动。
    void destructorDestroysBinding()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        TestObservableObject b;
        scene.addItem(&a);
        scene.addItem(&b);

        a.setPos(10, 20);
        b.setPos(110, 120);

        auto* line = BroadItem::ConnectionLine::create(&scene, &a, &b);
        QVERIFY(line != nullptr);

        // 确认初始绑定有效
        a.setPos(30, 40);
        QCoreApplication::processEvents();
        QCOMPARE(b.pos(), QPointF(130, 140));

        // 删除 line，绑定应随之销毁
        delete line;

        // 再次移动 a，b 应不再跟随
        QPointF bBefore = b.pos();
        a.setPos(0, 0);
        QCoreApplication::processEvents();
        QCOMPARE(b.pos(), bBefore);
    }

    /// @brief 测试端点销毁时 line 自动隐藏。
    void endpointDestructionHidesLine()
    {
        QGraphicsScene scene;

        auto* a = new TestObservableObject();
        auto* b = new TestObservableObject();
        scene.addItem(a);
        scene.addItem(b);

        a->setPos(0, 0);
        b->setPos(100, 0);

        auto* line = BroadItem::ConnectionLine::create(&scene, a, b);
        QVERIFY(line != nullptr);
        QVERIFY(line->isVisible());

        // 删除端点 a
        delete a;

        // line 应自动隐藏
        QVERIFY(!line->isVisible());

        // line 自身仍存活，可安全 delete
        delete line;
        delete b;
    }

    /// @brief 测试 setPen/pen 样式设置。
    void penStyling()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        TestObservableObject b;
        scene.addItem(&a);
        scene.addItem(&b);

        auto* line = BroadItem::ConnectionLine::create(&scene, &a, &b);
        QVERIFY(line != nullptr);

        line->setPen(QPen(Qt::red, 3));
        QCOMPARE(line->pen().color(), QColor(Qt::red));
        QCOMPARE(line->pen().width(), 3);

        delete line;
    }

    /// @brief 测试 zValue 为 1（线在端点上方）。
    void zOrderAboveEndpoints()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        TestObservableObject b;
        scene.addItem(&a);
        scene.addItem(&b);

        auto* line = BroadItem::ConnectionLine::create(&scene, &a, &b);
        QVERIFY(line != nullptr);
        QCOMPARE(line->zValue(), 1.0);

        delete line;
    }

    /// @brief 测试鼠标事件被忽略：acceptedMouseButtons 为 NoButton，不可选。
    void mouseIgnored()
    {
        QGraphicsScene scene;
        TestObservableObject a;
        TestObservableObject b;
        scene.addItem(&a);
        scene.addItem(&b);

        auto* line = BroadItem::ConnectionLine::create(&scene, &a, &b);
        QVERIFY(line != nullptr);

        QCOMPARE(line->acceptedMouseButtons(), Qt::NoButton);
        QVERIFY((line->flags() & QGraphicsItem::ItemIsSelectable) == 0);

        delete line;
    }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestConnectionLine)
#include "test_connection_line.moc"
