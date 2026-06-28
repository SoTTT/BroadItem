#include <QtTest/QtTest>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <broaditem/reactive/ReactiveBinding.h>
#include <broaditem/reactive/ReactiveProperty.h>
#include <broaditem/reactive/FollowBinding.h>
#include <broaditem/reactive/ConnectionLine.h>
#include <broaditem/reactive/AnchorDecorator.h>
#include <broaditem/reactive/AnchorPoint.h>
#include <broaditem/core/BroadItem.h>

#include <cmath>

/// @brief 最小化 QGraphicsObject 具体实现，用于测试绑定逻辑。
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

/// @brief 判断两个 QPointF 在容差范围内是否相等。
/// @param a 第一个点。
/// @param b 第二个点。
/// @param delta 允许的绝对误差（默认 0.5px）。
/// @return true 表示两点在容差内相等。
static bool pointsNear(const QPointF& a, const QPointF& b, qreal delta = 0.5)
{
    return std::abs(a.x() - b.x()) <= delta && std::abs(a.y() - b.y()) <= delta;
}

/// @brief 响应式绑定系统的综合测试套件。
///
/// 覆盖属性绑定、变换函数、循环检测、生命周期管理和 BroadItem 互操作。
class TestReactiveBinding : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    /// @brief 测试位置绑定：source.pos → target.pos。
    void testPositionBinding()
    {
        TestObservableObject source;
        TestObservableObject target;

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Pos,
                                                &target, BroadItem::Property::Pos);
        QVERIFY(binding != nullptr);
        QVERIFY(binding->isEnabled());

        source.setPos(100.0, 200.0);
        QCOMPARE(target.pos(), QPointF(100.0, 200.0));

        binding->destroy();
        delete binding;
    }

    /// @brief 测试缩放绑定：source.scale → target.scale。
    void testScaleBinding()
    {
        TestObservableObject source;
        TestObservableObject target;

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Scale,
                                                &target, BroadItem::Property::Scale);
        QVERIFY(binding != nullptr);

        source.setScale(2.5);
        QCOMPARE(target.scale(), 2.5);

        binding->destroy();
        delete binding;
    }

    /// @brief 测试旋转绑定：source.rotation → target.rotation。
    void testRotationBinding()
    {
        TestObservableObject source;
        TestObservableObject target;

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Rotation,
                                                &target, BroadItem::Property::Rotation);
        QVERIFY(binding != nullptr);

        source.setRotation(45.0);
        QCOMPARE(target.rotation(), 45.0);

        binding->destroy();
        delete binding;
    }

    /// @brief 测试透明度绑定：source.opacity → target.opacity。
    void testOpacityBinding()
    {
        TestObservableObject source;
        TestObservableObject target;

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Opacity,
                                                &target, BroadItem::Property::Opacity);
        QVERIFY(binding != nullptr);

        // Set a known non-default opacity first
        source.setOpacity(0.75);
        QCOMPARE(target.opacity(), 0.75);

        source.setOpacity(0.3);
        QCOMPARE(target.opacity(), 0.3);

        binding->destroy();
        delete binding;
    }

    /// @brief 测试可见性绑定：source.visible → target.visible。
    void testVisibilityBinding()
    {
        TestObservableObject source;
        TestObservableObject target;

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Visible,
                                                &target, BroadItem::Property::Visible);
        QVERIFY(binding != nullptr);

        // Initially both are visible
        QVERIFY(target.isVisible());

        source.setVisible(false);
        QVERIFY(!target.isVisible());

        source.setVisible(true);
        QVERIFY(target.isVisible());

        binding->destroy();
        delete binding;
    }

    /// @brief 测试自定义变换函数：偏移变换是否正确应用。
    void testTransformFunction()
    {
        TestObservableObject source;
        TestObservableObject target;

        auto offsetTransform = [](const QVariant& v) -> QVariant {
            return v.toPointF() + QPointF(10.0, 20.0);
        };

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Pos,
                                                &target, BroadItem::Property::Pos,
                                                offsetTransform);
        QVERIFY(binding != nullptr);

        source.setPos(100.0, 200.0);
        QCOMPARE(target.pos(), QPointF(110.0, 220.0));

        // Also test with scale via transform
        TestObservableObject source2;
        TestObservableObject target2;

        auto scaleDouble = [](const QVariant& v) -> QVariant {
            return v.value<qreal>() * 2.0;
        };

        auto* binding2 = BroadItem::ReactiveBinding::create(&source2, BroadItem::Property::Scale,
                                                 &target2, BroadItem::Property::Scale,
                                                 scaleDouble);
        QVERIFY(binding2 != nullptr);

        source2.setScale(3.0);
        QCOMPARE(target2.scale(), 6.0);

        binding->destroy();
        delete binding;
        binding2->destroy();
        delete binding2;
    }

    /// @brief 测试循环检测：A.pos → B.pos（带偏移变换）和 B.pos → A.pos 的双向绑定会触发循环检测并自动禁用。
    void testCycleDetection()
    {
        auto* a = new TestObservableObject();
        auto* b = new TestObservableObject();

        // bindingAB 带偏移变换，确保写入 B 的值与 B 回写给 A 的值不同，
        // 从而形成无限振荡以触发循环检测
        auto offsetTransform = [](const QVariant& v) -> QVariant {
            return v.toPointF() + QPointF(10.0, 0.0);
        };

        auto* bindingAB = BroadItem::ReactiveBinding::create(
            a, BroadItem::Property::Pos,
            b, BroadItem::Property::Pos,
            offsetTransform);
        auto* bindingBA = BroadItem::ReactiveBinding::create(
            b, BroadItem::Property::Pos,
            a, BroadItem::Property::Pos);
        QVERIFY(bindingAB != nullptr);
        QVERIFY(bindingBA != nullptr);
        QVERIFY(bindingAB->isEnabled());
        QVERIFY(bindingBA->isEnabled());

        // 监听 xChanged() 信号（pos 无 NOTIFY，通过 xChanged/yChanged 连接）
        QSignalSpy spyAX(a, &QGraphicsObject::xChanged);
        QSignalSpy spyBX(b, &QGraphicsObject::xChanged);

        // 触发循环：A 的位置变化经由 bindingAB（+10 偏移）写入 B，
        // B 的变化经由 bindingBA 回写到 A，因值不同形成无限循环
        a->setPos(100.0, 200.0);

        // 验证信号确实被触发
        QVERIFY(spyAX.count() >= 1);
        QVERIFY(spyBX.count() >= 1);

        // 至少有一个绑定被循环检测禁用
        QVERIFY(!bindingAB->isEnabled() || !bindingBA->isEnabled());

        bindingAB->destroy();
        bindingBA->destroy();
        delete bindingAB;
        delete bindingBA;
        delete a;
        delete b;
    }

    /// @brief 测试自绑定：源和目标为同一对象且属性相同时，create() 返回 nullptr。
    void testSelfBinding()
    {
        TestObservableObject obj;

        auto* binding = BroadItem::ReactiveBinding::create(&obj, BroadItem::Property::Pos,
                                                &obj, BroadItem::Property::Pos);
        QVERIFY(binding == nullptr);

        // 不同属性则允许（例如 pos → scale）
        auto* bindingDiff = BroadItem::ReactiveBinding::create(&obj, BroadItem::Property::Pos,
                                                    &obj, BroadItem::Property::Scale);
        QVERIFY(bindingDiff != nullptr);

        bindingDiff->destroy();
        delete bindingDiff;
    }

    /// @brief 测试源对象销毁：目标保持在销毁前的最后值，绑定自动禁用。
    void testSourceDestroyed()
    {
        auto* source = new TestObservableObject();
        auto* target = new TestObservableObject();

        auto* binding = BroadItem::ReactiveBinding::create(source, BroadItem::Property::Pos,
                                                target, BroadItem::Property::Pos);
        QVERIFY(binding != nullptr);
        QVERIFY(binding->isEnabled());

        // 先同步一次值
        source->setPos(42.0, 99.0);
        QCOMPARE(target->pos(), QPointF(42.0, 99.0));

        // 销毁源
        delete source;

        // 绑定应自动禁用
        QVERIFY(!binding->isEnabled());
        QVERIFY(binding->source() == nullptr);

        // 目标保持在最后值
        QCOMPARE(target->pos(), QPointF(42.0, 99.0));

        binding->destroy();
        delete binding;
        delete target;
    }

    /// @brief 测试目标对象销毁：不应崩溃，绑定自动禁用。
    void testTargetDestroyed()
    {
        auto* source = new TestObservableObject();
        auto* target = new TestObservableObject();

        auto* binding = BroadItem::ReactiveBinding::create(source, BroadItem::Property::Pos,
                                                target, BroadItem::Property::Pos);
        QVERIFY(binding != nullptr);
        QVERIFY(binding->isEnabled());

        source->setPos(10.0, 20.0);
        QCOMPARE(target->pos(), QPointF(10.0, 20.0));

        // 销毁目标
        delete target;

        // 绑定应自动禁用，不崩溃
        QVERIFY(!binding->isEnabled());
        QVERIFY(binding->target() == nullptr);

        // 再次操作源也不应崩溃
        source->setPos(30.0, 40.0);

        binding->destroy();
        delete binding;
        delete source;
    }

    /// @brief 测试启用/禁用：setEnabled(false) 暂停绑定，setEnabled(true) 恢复。
    void testEnableDisable()
    {
        TestObservableObject source;
        TestObservableObject target;

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Pos,
                                                &target, BroadItem::Property::Pos);
        QVERIFY(binding != nullptr);

        // 初始启用状态
        source.setPos(100.0, 200.0);
        QCOMPARE(target.pos(), QPointF(100.0, 200.0));

        // 禁用绑定
        binding->setEnabled(false);
        QVERIFY(!binding->isEnabled());

        // 源变化不应传播
        source.setPos(300.0, 400.0);
        QCOMPARE(target.pos(), QPointF(100.0, 200.0));

        // 重新启用绑定
        binding->setEnabled(true);
        QVERIFY(binding->isEnabled());

        // 再次改变源，应传播到目标
        source.setPos(500.0, 600.0);
        QCOMPARE(target.pos(), QPointF(500.0, 600.0));

        binding->destroy();
        delete binding;
    }

    /// @brief 测试无效属性名：create() 对无效属性名返回 nullptr。
    void testInvalidPropertyName()
    {
        TestObservableObject source;
        TestObservableObject target;

        // 无效源属性名
        auto* b1 = BroadItem::ReactiveBinding::create(&source, QStringLiteral("bogus"),
                                           &target, BroadItem::Property::Pos);
        QVERIFY(b1 == nullptr);

        // 无效目标属性名
        auto* b2 = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Pos,
                                           &target, QStringLiteral("unknown"));
        QVERIFY(b2 == nullptr);

        // 两个都无效
        auto* b3 = BroadItem::ReactiveBinding::create(&source, QStringLiteral("foo"),
                                           &target, QStringLiteral("bar"));
        QVERIFY(b3 == nullptr);

        // 空字符串
        auto* b4 = BroadItem::ReactiveBinding::create(&source, QString(),
                                           &target, BroadItem::Property::Pos);
        QVERIFY(b4 == nullptr);

        // nullptr 源
        auto* b5 = BroadItem::ReactiveBinding::create(nullptr, BroadItem::Property::Pos,
                                           &target, BroadItem::Property::Pos);
        QVERIFY(b5 == nullptr);

        // nullptr 目标
        auto* b6 = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Pos,
                                           nullptr, BroadItem::Property::Pos);
        QVERIFY(b6 == nullptr);
    }

    /// @brief 测试 BroadItem 作为绑定源（QObject* 接口）。
    void testBroadItemAsSource()
    {
        BroadItem::BroadItem source(QStringLiteral("test_layout.xml"));
        TestObservableObject target;

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Pos,
                                                &target, BroadItem::Property::Pos);
        QVERIFY(binding != nullptr);
        QVERIFY(binding->isEnabled());

        source.setPos(150.0, 250.0);
        QCOMPARE(target.pos(), QPointF(150.0, 250.0));

        source.setScale(1.5);
        auto* scaleBinding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Scale,
                                                     &target, BroadItem::Property::Scale);
        QVERIFY(scaleBinding != nullptr);
        source.setScale(2.0);
        QCOMPARE(target.scale(), 2.0);

        binding->destroy();
        delete binding;
        scaleBinding->destroy();
        delete scaleBinding;
    }

    /// @brief 测试 BroadItem 作为绑定目标（QObject* 接口）。
    void testBroadItemAsTarget()
    {
        TestObservableObject source;
        BroadItem::BroadItem target(QStringLiteral("test_layout.xml"));

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Pos,
                                                &target, BroadItem::Property::Pos);
        QVERIFY(binding != nullptr);
        QVERIFY(binding->isEnabled());

        source.setPos(88.0, 77.0);
        QCOMPARE(target.pos(), QPointF(88.0, 77.0));

        source.setOpacity(0.4);
        auto* opacityBinding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Opacity,
                                                       &target, BroadItem::Property::Opacity);
        QVERIFY(opacityBinding != nullptr);
        source.setOpacity(0.6);
        QCOMPARE(target.opacity(), 0.6);

        binding->destroy();
        delete binding;
        opacityBinding->destroy();
        delete opacityBinding;
    }

    /// @brief 测试 pos 分量变化：只改 x 时 target 同步更新。
    ///
    /// 验证 pos 绑定通过 xChanged()/yChanged() 特判连接在只改变 x 分量时仍能触发目标更新。
    void testPosComponentChange()
    {
        TestObservableObject source;
        TestObservableObject target;

        auto* binding = BroadItem::ReactiveBinding::create(&source, BroadItem::Property::Pos,
                                                &target, BroadItem::Property::Pos);
        QVERIFY(binding != nullptr);

        // 初始位置
        source.setPos(100.0, 200.0);
        QCOMPARE(target.pos(), QPointF(100.0, 200.0));

        // 只改变 x 分量，验证 target 同步
        source.setX(300.0);
        QCOMPARE(target.pos(), QPointF(300.0, 200.0));

        // 只改变 y 分量，验证 target 同步
        source.setY(400.0);
        QCOMPARE(target.pos(), QPointF(300.0, 400.0));

        // 同时改变两个分量
        source.setPos(500.0, 600.0);
        QCOMPARE(target.pos(), QPointF(500.0, 600.0));

        binding->destroy();
        delete binding;
    }

    /// @brief 测试观察者模式：源属性变化时回调被执行，不写入目标。
    void testObserverCallback()
    {
        TestObservableObject source;
        QPointF observedPos;

        auto* binding = BroadItem::ReactiveBinding::createObserver(
            &source, BroadItem::Property::Pos,
            [&observedPos](const QVariant& v) -> QVariant {
                observedPos = v.toPointF();
                return {};
            });
        QVERIFY(binding != nullptr);

        source.setPos(42.0, 99.0);
        QCOMPARE(observedPos, QPointF(42.0, 99.0));

        source.setPos(123.0, 456.0);
        QCOMPARE(observedPos, QPointF(123.0, 456.0));

        binding->destroy();
        delete binding;
    }

    /// @brief 测试 FollowBinding：拖动 follower 后 leader 移动应保持新偏移。
    void testFollowBindingOffsetUpdate()
    {
        TestObservableObject leader;
        TestObservableObject follower;

        QPointF initialOffset(120.0, 80.0);
        leader.setPos(50.0, 170.0);
        follower.setPos(leader.pos() + initialOffset);

        auto* binding = BroadItem::FollowBinding::create(
            &leader, &follower, initialOffset);
        QVERIFY(binding != nullptr);

        // 拖动 follower 到 (200, 300)，偏移应更新为 (150, 130)
        follower.setPos(200.0, 300.0);
        QCOMPARE(binding->offset(), QPointF(150.0, 130.0));

        // 移动 leader 到 (60, 180)，follower 应保持新偏移
        leader.setPos(60.0, 180.0);
        QCOMPARE(follower.pos(), QPointF(210.0, 310.0));
        QCOMPARE(binding->offset(), QPointF(150.0, 130.0));

        binding->destroy();
        delete binding;
    }

    /// @brief 测试 createScenePosObserver 在嵌套 parent 场景下的行为。
    ///
    /// 目标对象初始挂在 parentA 下，移动 parentA 后回调应得到正确的 scenePos；
    /// 运行时重新父级化到 parentB 后，tracker 应重建父链并继续响应 parentB 移动。
    void testScenePosObserverNestedParent()
    {
        QGraphicsScene scene;
        auto* parentA = new TestObservableObject();
        auto* parentB = new TestObservableObject();
        auto* child = new TestObservableObject(parentA);
        scene.addItem(parentA);
        scene.addItem(parentB);

        child->setPos(10.0, 10.0);

        QPointF observedPos;
        auto* binding = BroadItem::ReactiveBinding::createScenePosObserver(
            child,
            [&observedPos](const QVariant& v) -> QVariant {
                observedPos = v.toPointF();
                return {};
            });
        QVERIFY(binding != nullptr);
        QVERIFY(binding->isEnabled());

        parentA->setPos(100.0, 100.0);
        QCOMPARE(observedPos, QPointF(110.0, 110.0));

        child->setParentItem(parentB);
        parentB->setPos(200.0, 50.0);
        QCOMPARE(observedPos, QPointF(210.0, 60.0));

        binding->destroy();
        delete binding;
        delete child;
        delete parentA;
        delete parentB;
    }

    /// @brief 测试跨 parent chain 的 FollowBinding 跟随。
    ///
    /// leader 与 follower 分别位于不同父节点链下，移动 leader 的父节点时，
    /// follower 应保持固定的场景坐标偏移。
    void testFollowBindingNestedParents()
    {
        QGraphicsScene scene;
        auto* leaderParent = new TestObservableObject();
        auto* followerParent = new TestObservableObject();
        auto* leader = new TestObservableObject(leaderParent);
        auto* follower = new TestObservableObject(followerParent);
        scene.addItem(leaderParent);
        scene.addItem(followerParent);

        leader->setPos(10.0, 20.0);
        follower->setPos(30.0, 40.0);

        auto* binding = BroadItem::FollowBinding::create(
            leader, follower, QPointF());
        QVERIFY(binding != nullptr);

        const QPointF expectedOffset = follower->scenePos() - leader->scenePos();

        leaderParent->setPos(100.0, 0.0);
        QCOMPARE(follower->scenePos(), leader->scenePos() + expectedOffset);

        binding->destroy();
        delete binding;
        delete leader;
        delete follower;
        delete leaderParent;
        delete followerParent;
    }

    /// @brief 测试祖先销毁时场景位置观察者的安全清理。
    ///
    /// 将 child 先从即将被删除的中间祖先 parentA 重新父级化到 parentRoot，
    /// 再删除 parentA；observer 不应崩溃，且能继续响应剩余 parent chain。
    void testScenePosObserverAncestorDestroy()
    {
        QGraphicsScene scene;
        auto* parentRoot = new TestObservableObject();
        auto* parentA = new TestObservableObject(parentRoot);
        auto* child = new TestObservableObject(parentA);
        scene.addItem(parentRoot);

        child->setPos(10.0, 10.0);

        QPointF observedPos;
        auto* binding = BroadItem::ReactiveBinding::createScenePosObserver(
            child,
            [&observedPos](const QVariant& v) -> QVariant {
                observedPos = v.toPointF();
                return {};
            });
        QVERIFY(binding != nullptr);
        QVERIFY(binding->isEnabled());

        // 将 child 提前从 parentA 下解离，挂到 parentRoot，避免被级联删除
        child->setParentItem(parentRoot);
        QVERIFY(binding->isEnabled());

        // 删除中间祖先 parentA，observer 不应崩溃
        delete parentA;
        QVERIFY(binding->isEnabled());

        parentRoot->setPos(50.0, 50.0);
        QCOMPARE(observedPos, QPointF(60.0, 60.0));

        binding->destroy();
        delete binding;
        delete child;
        delete parentRoot;
    }

    /// @brief 测试 createScenePosObserver 基础场景：顶层目标移动时 callback 被调用并返回正确 scenePos。
    void testScenePosObserverBasic()
    {
        QGraphicsScene scene;
        auto* target = new TestObservableObject();
        scene.addItem(target);

        QPointF observedPos;
        auto* binding = BroadItem::ReactiveBinding::createScenePosObserver(
            target,
            [&observedPos](const QVariant& v) -> QVariant {
                observedPos = v.toPointF();
                return {};
            });
        QVERIFY(binding != nullptr);
        QVERIFY(binding->isEnabled());

        target->setPos(123.0, 456.0);
        QCOMPARE(observedPos, QPointF(123.0, 456.0));

        binding->destroy();
        delete binding;
        delete target;
    }

    /// @brief 测试 createScenePosObserver 对祖先 scale/rotation 变化的响应。
    ///
    /// child 挂在 parent 下，改变 parent 的 scale 与 rotation 后，
    /// callback 应被触发且最终 scenePos 按变换更新。
    void testScenePosObserverAncestorScaleRotation()
    {
        QGraphicsScene scene;
        auto* parent = new TestObservableObject();
        auto* child = new TestObservableObject(parent);
        scene.addItem(parent);

        child->setPos(10.0, 0.0);

        int callbackCount = 0;
        QPointF observedPos;
        auto* binding = BroadItem::ReactiveBinding::createScenePosObserver(
            child,
            [&callbackCount, &observedPos](const QVariant& v) -> QVariant {
                ++callbackCount;
                observedPos = v.toPointF();
                return {};
            });
        QVERIFY(binding != nullptr);

        parent->setPos(100.0, 0.0);
        parent->setScale(2.0);
        parent->setRotation(90.0);

        QVERIFY(callbackCount > 0);
        // child 局部 (10,0) 经 scale=2 得 (20,0)，再旋转 90° 得 (0,20)
        QVERIFY2(pointsNear(observedPos, QPointF(100.0, 20.0)),
                 qPrintable(QString("expected (100,20), got (%1,%2)")
                            .arg(observedPos.x()).arg(observedPos.y())));

        binding->destroy();
        delete binding;
        delete child;
        delete parent;
    }

    /// @brief 测试 ConnectionLine 在嵌套 parent 链下的端点跟随。
    ///
    /// 两个端点分别挂在不同父节点下，移动任一父节点后，
    /// 连接线 boundingRect 中心应等于两端 scenePos 的中点。
    void testConnectionLineNested()
    {
        QGraphicsScene scene;
        auto* parentA = new TestObservableObject();
        auto* parentB = new TestObservableObject();
        auto* endpointA = new TestObservableObject(parentA);
        auto* endpointB = new TestObservableObject(parentB);
        scene.addItem(parentA);
        scene.addItem(parentB);

        endpointA->setPos(10.0, 0.0);
        endpointB->setPos(0.0, 10.0);

        auto* line = BroadItem::ConnectionLine::create(
            &scene, endpointA, endpointB);
        QVERIFY(line != nullptr);
        QVERIFY(line->isVisible());

        parentA->setPos(50.0, 50.0);
        QCoreApplication::processEvents();

        QVERIFY(line->isVisible());

        QPointF mid = (endpointA->scenePos() + endpointB->scenePos()) / 2.0;
        QRectF br = line->boundingRect();
        QPointF center(br.center());
        QVERIFY2(pointsNear(center, mid),
                 qPrintable(QString("line center expected (%1,%2), got (%3,%4)")
                            .arg(mid.x()).arg(mid.y())
                            .arg(center.x()).arg(center.y())));

        delete line;
        delete endpointA;
        delete endpointB;
        delete parentA;
        delete parentB;
    }

    /// @brief 测试 AnchorDecorator 在已有父节点链中的跟随行为。
    ///
    /// 装饰已有 parent 的子对象后移动 parent group，
    /// 验证装饰器与锚点 scenePos 同步移动，且 decorated 位置保持连贯。
    void testAnchorDecoratorNested()
    {
        QGraphicsScene scene;
        auto* parent = new TestObservableObject();
        auto* child = new TestObservableObject(parent);
        scene.addItem(parent);

        child->setPos(10.0, 20.0);

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, child);
        QVERIFY(decorator != nullptr);

        QPointF decoratedBefore = child->scenePos();
        QPointF decoratorSceneBefore = decorator->scenePos();
        auto* nAnchor = decorator->anchor(BroadItem::AnchorDecorator::AnchorSide::North);
        QVERIFY(nAnchor != nullptr);
        QPointF anchorBefore = nAnchor->pos();

        parent->setPos(50.0, 50.0);
        QCoreApplication::processEvents();

        QPointF decoratedAfter = child->scenePos();
        QPointF decoratorSceneAfter = decorator->scenePos();
        QPointF anchorAfter = nAnchor->pos();

        QVERIFY2(pointsNear(decoratedAfter, decoratedBefore + QPointF(50.0, 50.0)),
                 "decorated should move with parent group");
        QVERIFY2(pointsNear(decoratorSceneAfter, decoratorSceneBefore + QPointF(50.0, 50.0)),
                 "decorator should move with parent group");
        QVERIFY2(pointsNear(anchorAfter, anchorBefore + QPointF(50.0, 50.0)),
                 "anchor should move with parent group");

        delete decorator;
        delete parent;
    }

    /// @brief 测试 AnchorDecorator 析构后 decorated 恢复到原 parent 且 scenePos 不变。
    void testAnchorDecoratorRestoreOnDestroy()
    {
        QGraphicsScene scene;
        auto* parent = new TestObservableObject();
        scene.addItem(parent);
        parent->setPos(30.0, 40.0);

        auto* child = new TestObservableObject(parent);
        child->setPos(10.0, 20.0);

        QPointF scenePosBefore = child->scenePos();

        auto* decorator = BroadItem::AnchorDecorator::create(&scene, child);
        QVERIFY(decorator != nullptr);
        QCOMPARE(child->parentItem(), static_cast<QGraphicsItem*>(decorator));

        delete decorator;

        QCOMPARE(child->parentItem(), static_cast<QGraphicsItem*>(parent));
        QVERIFY2(pointsNear(child->scenePos(), scenePosBefore),
                 "decorated scene position should be preserved after decorator deletion");

        delete parent;
    }

    /// @brief 测试跨 parent chain 的 FollowBinding 跟随。
    ///
    /// leader 与 follower 分别位于不同父节点链下，移动 leader 的父节点时，
    /// follower 应保持固定的场景坐标偏移。
    void testFollowBindingNested()
    {
        QGraphicsScene scene;
        auto* leaderParent = new TestObservableObject();
        auto* followerParent = new TestObservableObject();
        auto* leader = new TestObservableObject(leaderParent);
        auto* follower = new TestObservableObject(followerParent);
        scene.addItem(leaderParent);
        scene.addItem(followerParent);

        leader->setPos(10.0, 20.0);
        follower->setPos(30.0, 40.0);

        auto* binding = BroadItem::FollowBinding::create(
            leader, follower, QPointF());
        QVERIFY(binding != nullptr);

        const QPointF expectedOffset = follower->scenePos() - leader->scenePos();

        leaderParent->setPos(100.0, 0.0);
        QCoreApplication::processEvents();

        QCOMPARE(follower->scenePos(), leader->scenePos() + expectedOffset);

        binding->destroy();
        delete binding;
        delete leader;
        delete follower;
        delete leaderParent;
        delete followerParent;
    }

    /// @brief 测试嵌套 parent 下拖动 follower 后 offset 更新。
    ///
    /// 先手动移动 follower（模拟拖拽），再移动 leader 的父节点，
    /// 验证 follower 使用新的场景坐标偏移。
    void testFollowBindingDragUpdatesOffsetNested()
    {
        QGraphicsScene scene;
        auto* leaderParent = new TestObservableObject();
        auto* followerParent = new TestObservableObject();
        auto* leader = new TestObservableObject(leaderParent);
        auto* follower = new TestObservableObject(followerParent);
        scene.addItem(leaderParent);
        scene.addItem(followerParent);

        leader->setPos(10.0, 20.0);
        follower->setPos(30.0, 40.0);

        auto* binding = BroadItem::FollowBinding::create(
            leader, follower, QPointF());
        QVERIFY(binding != nullptr);

        // 拖动 follower 到 (200, 300)，偏移应更新
        follower->setPos(200.0, 300.0);
        QCoreApplication::processEvents();

        const QPointF newOffset = follower->scenePos() - leader->scenePos();
        QCOMPARE(binding->offset(), newOffset);

        // 移动 leader 的父节点，follower 应保持新偏移
        leaderParent->setPos(100.0, 0.0);
        QCoreApplication::processEvents();

        QCOMPARE(follower->scenePos(), leader->scenePos() + newOffset);

        binding->destroy();
        delete binding;
        delete leader;
        delete follower;
        delete leaderParent;
        delete followerParent;
    }

    /// @brief 测试祖先重新父级化后 ScenePosObserver 继续正确工作。
    ///
    /// 创建 grandParent -> parentA -> child 链；创建 observer；
    /// 将 parentA 重新父级化到新的 parentB（不销毁 parentA）；
    /// 移动 parentB，验证 callback 得到正确 scenePos。
    void testScenePosObserverAncestorReparent()
    {
        QGraphicsScene scene;
        auto* grandParent = new TestObservableObject();
        auto* parentA = new TestObservableObject(grandParent);
        auto* parentB = new TestObservableObject(grandParent);
        auto* child = new TestObservableObject(parentA);
        scene.addItem(grandParent);

        child->setPos(10.0, 10.0);

        QPointF observedPos;
        auto* binding = BroadItem::ReactiveBinding::createScenePosObserver(
            child,
            [&observedPos](const QVariant& v) -> QVariant {
                observedPos = v.toPointF();
                return {};
            });
        QVERIFY(binding != nullptr);
        QVERIFY(binding->isEnabled());

        // 将 parentA 重新父级化到 parentB，不销毁 parentA
        parentA->setParentItem(parentB);
        QVERIFY(binding->isEnabled());

        // 移动新的父链 parentB
        parentB->setPos(100.0, 100.0);
        QCOMPARE(observedPos, QPointF(110.0, 110.0));

        binding->destroy();
        delete binding;
        delete child;
        delete parentA;
        delete parentB;
        delete grandParent;
    }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestReactiveBinding)
#include "test_reactive_binding.moc"
