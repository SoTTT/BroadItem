#include <QtTest/QtTest>
#include <QApplication>
#include <QFont>
#include <QTemporaryFile>
#include <QDir>

#include <broaditem/core/Frame.h>
#include <broaditem/core/LayoutEngine.h>
#include <broaditem/core/Node.h>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/parser/LayoutRegistry.h>
#include <broaditem/element/text/TextElement.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>

using namespace BroadItem;

/// @brief 逐像素比较两张图（容差 0），尺寸不同直接判异。
/// @param a 图 A。
/// @param b 图 B。
/// @return 完全一致返回 true，否则 false。
static bool imagesEqual(const QImage& a, const QImage& b)
{
    if (a.size() != b.size())
        return false;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x)
            if (a.pixel(x, y) != b.pixel(x, y))
                return false;
    return true;
}

/// @brief 递归收集实例节点树中所有 TextNode 的解析文本。
/// @param node 实例节点（可为 nullptr）。
/// @param[out] out 输出文本列表。
static void collectTexts(const BroadItem::Node* node, QStringList& out)
{
    if (!node)
        return;
    if (const auto* tn = dynamic_cast<const BroadItem::TextNode*>(node))
        out << tn->text;
    for (const auto& c : node->children)
        collectTexts(c.get(), out);
}

class TestRegression : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    /// @brief testRegistryIsolation：LayoutRegistry 多实例隔离（for 布局）。
    ///
    /// 同一注册布局被两个 Frame（不同属性上下文）实例化时，
    /// 先创建的实例渲染结果不得被后创建的实例踩踏。
    /// 布局根 column 内含 \<for b:of="items" b:as="it"\> 展开 text；
    /// A 的 items={甲}，B 的 items={乙,丙}（列表长度不同，尺寸与内容双重差异）。
    ///
    /// 旧代码必败论证（精确机制）：旧架构 LayoutRegistry 返回共享 Element 树，
    /// MultiChildContainer::m_flattened 是容器上的共享可变缓存；
    /// B 的 measure/layout 以 B 的数据覆写共享树的展平列表
    /// （ColumnLayout::measure 与 layout 均执行 m_flattened = flattenChildren(ctx)，
    /// 克隆文本已在 ForElement::expand 期以 B 的 per-item 上下文烘焙），
    /// A 的 render 遍历到 B 的克隆实例（旧 MultiChildContainer::render 迭代 m_flattened）。
    /// 注：旧表述"resolveBindings 改写共享节点 m_text"对无 for 布局不成立——
    /// 静态树文本是 render 时经 resolvedText(ctx) live-lookup 的，各实例各取所需，
    /// 不构成踩踏通道；故本测试改用 for 布局，使共享可变状态 m_flattened
    /// 成为踩踏通道。新架构模板树不可变，展开产物落在各自的实例节点树，天然隔离。
    ///
    /// 实证（9dc3a53 worktree 对照实验，同布局/同数据/同字体/同顺序）：
    /// imgA(11x17) != imgRef(11x17)，重叠区差 103 px（A 被 B 踩踏）；
    /// imgA 与 imgB(12x34) 重叠区差 0 px（A 渲染的确为 B 的克隆实例）。
    void testRegistryIsolation()
    {
        // 固定应用字体，保证渲染结果确定（逐像素比较的前提）。
        // PingFang SC 是 macOS 系统中文字体——本测试渲染中文文本且仅在本机
        // macOS 环境运行，故硬编码该字体以避免字体回退带来的像素差异。
        QApplication::setFont(QFont(QStringLiteral("PingFang SC")));

        const QString xml = QStringLiteral(
            "<root xmlns:b=\"urn:broaditem:binding\">"
            "<column>"
            "<for b:of=\"items\" b:as=\"it\">"
            "<text font-size=\"12\" b:content=\"it\"/>"
            "</for>"
            "</column>"
            "</root>");

        auto root = BroadItem::XmlLayoutParser::parseString(xml);
        QVERIFY(root != nullptr);
        BroadItem::LayoutRegistry::instance().registerLayout(90001, root);

        // 先建 A 再建 B（空上下文），再设属性：A 先、B 后。
        // 实例 A：items={甲}（先创建）
        auto ctxA = std::make_shared<BroadItem::MapPropertyContext>();
        auto frameA = BroadItem::Frame::fromRegistry(90001, ctxA);
        QVERIFY(frameA != nullptr);

        // 实例 B：items={乙,丙}（后创建，列表长度不同；
        // 旧代码中 B 的 measure/layout 覆写共享树的 m_flattened）
        auto ctxB = std::make_shared<BroadItem::MapPropertyContext>();
        auto frameB = BroadItem::Frame::fromRegistry(90001, ctxB);
        QVERIFY(frameB != nullptr);

        frameA->setDynamicProperty(QStringLiteral("items"),
                                   QStringList{QStringLiteral("甲")});
        frameB->setDynamicProperty(QStringLiteral("items"),
                                   QStringList{QStringLiteral("乙"),
                                               QStringLiteral("丙")});
        // 默认 Coalesced 策略下立即执行待定重布局
        frameA->flush();
        frameB->flush();

        // 参照物：同一 XML 落临时文件 + items={甲}，走 fromFile 独立渲染。
        QTemporaryFile tmp(QDir::tempPath() + QStringLiteral("/bi_reg_XXXXXX.xml"));
        QVERIFY(tmp.open());
        tmp.write(xml.toUtf8());
        tmp.flush();
        auto ctxRef = std::make_shared<BroadItem::MapPropertyContext>();
        auto frameRef = BroadItem::Frame::fromFile(tmp.fileName(), ctxRef);
        QVERIFY(frameRef != nullptr);
        frameRef->setDynamicProperty(QStringLiteral("items"),
                                     QStringList{QStringLiteral("甲")});
        frameRef->flush();

        // 先建 B 再渲染 A：旧代码下 A 的 render 必遍历到 B 的克隆实例。
        const QImage imgA = frameA->toImage(1.0);
        const QImage imgB = frameB->toImage(1.0);
        const QImage imgRef = frameRef->toImage(1.0);
        QVERIFY(!imgA.isNull());
        QVERIFY(!imgB.isNull());
        QVERIFY(!imgRef.isNull());

        // 断言 1：A 未被 B 踩踏 —— 与参照物逐像素一致（结构不变）。
        QVERIFY2(imagesEqual(imgA, imgRef),
                 "fromRegistry(A) 与 fromFile 参照物渲染不一致：A 被 B 踩踏（Registry 实例未隔离）");
        // 断言 2：A 与 B 渲染结果确实不同（排除"绑定根本没生效"的假阳性）。
        QVERIFY2(!imagesEqual(imgA, imgB),
                 "A 与 B 渲染结果相同：绑定未生效或列表相同，测试无效");
    }

    /// @brief testCellForStacksVertically：\<cell\> content 带 b:of 时垂直堆叠。
    ///
    /// cell 的 content（column 带 b:of）在物化期展开为 N 个实例节点，
    /// measure 累加高度、layout 依次分配，三个 column 矩形垂直堆叠不重叠。
    ///
    /// 旧代码必败论证：旧 CellElement（ContainerElement）不展开控制元素，
    /// ForElement::render 只画第一项 —— cell 节点 children.size() == 1
    /// 而非 3，且只渲染第一行文本，本测试的计数与堆叠断言全部失败。
    void testCellForStacksVertically()
    {
        const QString xml = QStringLiteral(
            "<root xmlns:b=\"urn:broaditem:binding\">"
            "<grid columns=\"1\" rows=\"1\">"
            "<cell padding=\"8\">"
            "<column space=\"4\" b:of=\"lines\" b:as=\"ln\">"
            "<text font-size=\"12\" b:content=\"ln\"/>"
            "</column>"
            "</cell>"
            "</grid>"
            "</root>");

        auto root = BroadItem::XmlLayoutParser::parseString(xml);
        QVERIFY(root != nullptr);

        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty(QStringLiteral("lines"),
                           QStringList{QStringLiteral("第一行"),
                                       QStringLiteral("第二行"),
                                       QStringLiteral("第三行")});
        BroadItem::LayoutContext lctx{&mapCtx};

        auto node = BroadItem::LayoutEngine::materialize(root, lctx);
        QVERIFY(node != nullptr);

        // grid 下 1 个 cell 节点；cell 下 3 个 column 节点（b:of 展开）。
        QCOMPARE(node->children.size(), size_t(1));
        const BroadItem::Node* cell = node->children[0].get();
        QCOMPARE(cell->children.size(), size_t(3));

        // 测量后以测量结果作为布局矩形，避免裁剪干扰。
        const QSizeF measured = BroadItem::LayoutEngine::measure(
            lctx, BroadItem::LayoutConstraints{}, *node);
        BroadItem::LayoutEngine::layout(
            lctx, QRectF(QPointF(0, 0), measured), *node);

        const QRectF r0 = cell->children[0]->rect;
        const QRectF r1 = cell->children[1]->rect;
        const QRectF r2 = cell->children[2]->rect;

        // 三个 column 节点垂直堆叠：y 严格递增且不重叠。
        QVERIFY2(r0.y() < r1.y() && r1.y() < r2.y(),
                 QStringLiteral("y 未严格递增: %1, %2, %3")
                     .arg(r0.y()).arg(r1.y()).arg(r2.y()).toUtf8());
        QVERIFY2(r0.bottom() <= r1.top(),
                 QStringLiteral("第 1/2 项重叠: [%1,%2] vs %3")
                     .arg(r0.top()).arg(r0.bottom()).arg(r1.top()).toUtf8());
        QVERIFY2(r1.bottom() <= r2.top(),
                 QStringLiteral("第 2/3 项重叠: [%1,%2] vs %3")
                     .arg(r1.top()).arg(r1.bottom()).arg(r2.top()).toUtf8());
    }

    /// @brief testNestedForExpands：嵌套 \<for\> 在物化期按 per-item 上下文展开。
    ///
    /// 外层 for 迭代 groups，内层 for 以"g.members"为数据源，
    /// 物化后全部 TextNode 文本集合恰为 {第一组,甲,乙,第二组,丙}。
    ///
    /// 旧代码必败论证：旧架构内层 for 在 measure 期以全局 ctx 展开，
    /// 全局上下文不存在"g.members"（g 是外层 for 的循环变量，只在
    /// per-item 上下文中有效）→ 内层恒为空，只收集到 {第一组,第二组}，
    /// 计数与集合断言失败。
    void testNestedForExpands()
    {
        const QString xml = QStringLiteral(
            "<root xmlns:b=\"urn:broaditem:binding\">"
            "<column>"
            "<for b:of=\"groups\" b:as=\"g\">"
            "<column>"
            "<text b:content=\"g.name\"/>"
            "<for b:of=\"g.members\" b:as=\"m\">"
            "<text b:content=\"m\"/>"
            "</for>"
            "</column>"
            "</for>"
            "</column>"
            "</root>");

        auto root = BroadItem::XmlLayoutParser::parseString(xml);
        QVERIFY(root != nullptr);

        QVariantMap g1;
        g1[QStringLiteral("name")] = QStringLiteral("第一组");
        g1[QStringLiteral("members")] = QStringList{QStringLiteral("甲"), QStringLiteral("乙")};
        QVariantMap g2;
        g2[QStringLiteral("name")] = QStringLiteral("第二组");
        g2[QStringLiteral("members")] = QStringList{QStringLiteral("丙")};

        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty(QStringLiteral("groups"), QVariantList{g1, g2});
        BroadItem::LayoutContext lctx{&mapCtx};

        auto node = BroadItem::LayoutEngine::materialize(root, lctx);
        QVERIFY(node != nullptr);

        QStringList actual;
        collectTexts(node.get(), actual);
        actual.sort();

        QStringList expected{QStringLiteral("第一组"), QStringLiteral("甲"),
                             QStringLiteral("乙"), QStringLiteral("第二组"),
                             QStringLiteral("丙")};
        expected.sort();

        QCOMPARE(actual.size(), 5);
        QCOMPARE(actual, expected);
    }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestRegression)
#include "test_regression.moc"
