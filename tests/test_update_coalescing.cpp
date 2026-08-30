#include <QtTest/QtTest>
#include <broaditem/core/Frame.h>

#include "helpers/frame_helpers.h"

using namespace BroadItem;

/// @brief 以绑定 b:content="msg" 的最简布局构造 Frame 并安装计数重布局动作。
///
/// Frame 构造期已完成首次布局（计数 hook 尚未安装），返回后调用方应将
/// 计数器清零再开始断言。
class CoalescingFixture {
public:
    /// @brief 构造绑定布局的 Frame。
    /// @return 解析并布局完成的 Frame。
    std::unique_ptr<Frame> makeFrame()
    {
        return TestHelpers::frameFromXmlString(QStringLiteral("<text b:content=\"msg\"/>"));
    }

    int relayoutCount = 0;  ///< 重布局动作执行次数（由 installCounter 注入的 hook 累加）。

    /// @brief 安装计数重布局动作（包装 performLayout，保持真实布局语义）。
    /// @param frame 目标 Frame。
    void installCounter(Frame* frame)
    {
        frame->setRelayoutAction([this, frame] {
            ++relayoutCount;
            frame->performLayout();
        });
        relayoutCount = 0;
    }
};

class TestUpdateCoalescing : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    /// @brief 用例1：默认 Coalesced——同回合 3 次绑定变更只触发一次重布局。
    void coalescedMergesMultipleChanges()
    {
        CoalescingFixture fx;
        auto frame = fx.makeFrame();
        QVERIFY2(frame, "Frame::fromFile 失败");
        fx.installCounter(frame.get());

        for (int i = 0; i < 3; ++i)
            frame->setDynamicProperty(QStringLiteral("msg"), QStringLiteral("v%1").arg(i));

        QCOMPARE(fx.relayoutCount, 0);            // 合并策略下不立即重布局
        QVERIFY(frame->hasPendingUpdate());

        QTRY_COMPARE(fx.relayoutCount, 1);        // 事件循环触发一次
        QTest::qWait(50);
        QCOMPARE(fx.relayoutCount, 1);            // 且仅一次
        QVERIFY(!frame->hasPendingUpdate());
    }

    /// @brief 用例2：flush() 立即执行待定更新，随后的定时任务被守卫位拦截。
    void flushExecutesImmediately()
    {
        CoalescingFixture fx;
        auto frame = fx.makeFrame();
        QVERIFY2(frame, "Frame::fromFile 失败");
        fx.installCounter(frame.get());

        frame->setDynamicProperty(QStringLiteral("msg"), QStringLiteral("x"));
        QVERIFY(frame->hasPendingUpdate());

        frame->flush();
        QCOMPARE(fx.relayoutCount, 1);            // 不等事件循环，立即生效
        QVERIFY(!frame->hasPendingUpdate());

        QTest::qWait(50);                          // 让已排入的定时任务触发
        QCOMPARE(fx.relayoutCount, 1);            // 守卫位拦截，不重复布局
    }

    /// @brief 用例3：Synchronous 策略——变更立即重布局，不排入事件循环。
    void synchronousPolicy()
    {
        CoalescingFixture fx;
        auto frame = fx.makeFrame();
        QVERIFY2(frame, "Frame::fromFile 失败");
        fx.installCounter(frame.get());

        frame->setUpdatePolicy(UpdatePolicy::Synchronous);
        frame->setDynamicProperty(QStringLiteral("msg"), QStringLiteral("x"));

        QCOMPARE(fx.relayoutCount, 1);
        QVERIFY(!frame->hasPendingUpdate());
    }

    /// @brief 用例4：未绑定属性变更不调度重布局。
    void unboundPropertyNotScheduled()
    {
        CoalescingFixture fx;
        auto frame = fx.makeFrame();
        QVERIFY2(frame, "Frame::fromFile 失败");
        fx.installCounter(frame.get());

        frame->setDynamicProperty(QStringLiteral("unbound"), QStringLiteral("x"));

        QCOMPARE(fx.relayoutCount, 0);
        QVERIFY(!frame->hasPendingUpdate());
    }

    /// @brief 用例5：flush() 幂等——无待定更新时调用无副作用。
    void flushIsIdempotent()
    {
        CoalescingFixture fx;
        auto frame = fx.makeFrame();
        QVERIFY2(frame, "Frame::fromFile 失败");
        fx.installCounter(frame.get());

        frame->flush();                            // 无待定更新
        QCOMPARE(fx.relayoutCount, 0);

        frame->setDynamicProperty(QStringLiteral("msg"), QStringLiteral("x"));
        frame->flush();
        frame->flush();                            // 重复调用
        QCOMPARE(fx.relayoutCount, 1);
    }

    /// @brief 用例6：跨事件循环回合各自触发——守卫位在任务执行后正确复位。
    void separateRoundsEachTrigger()
    {
        CoalescingFixture fx;
        auto frame = fx.makeFrame();
        QVERIFY2(frame, "Frame::fromFile 失败");
        fx.installCounter(frame.get());

        frame->setDynamicProperty(QStringLiteral("msg"), QStringLiteral("a"));
        QTRY_COMPARE(fx.relayoutCount, 1);

        frame->setDynamicProperty(QStringLiteral("msg"), QStringLiteral("b"));
        QVERIFY(frame->hasPendingUpdate());
        QTRY_COMPARE(fx.relayoutCount, 2);
    }
    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestUpdateCoalescing)
#include "test_update_coalescing.moc"
