#include <QtTest/QtTest>
#include <broaditem/element/control/ForElement.h>
#include <broaditem/element/control/IfHasElement.h>
#include <broaditem/element/text/TextElement.h>
#include <broaditem/element/layout/RowLayout.h>
#include <broaditem/element/layout/ColumnLayout.h>
#include <broaditem/element/layout/GridLayout.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <QDomDocument>
#include <QDomElement>

/// @brief 以编程方式创建带 b:content 绑定的 TextElement 模板。
/// @param binding 绑定路径（如 "n"、"p.name"）。
/// @param fontSize 字体大小（点）。
/// @return 解析完成的 TextElement 共享指针。
static std::shared_ptr<BroadItem::TextElement> makeBoundText(const QString& binding, const QString& fontSize = QStringLiteral("11"))
{
    QDomDocument doc;
    QDomElement el = doc.createElement("text");
    el.setAttributeNS(BroadItem::BINDING_NS, "b:content", binding);
    el.setAttribute("font-size", fontSize);
    auto te = std::make_shared<BroadItem::TextElement>();
    te->parse(el);
    return te;
}

/// @brief 提取实例节点的解析文本。
/// @param n 由 TextElement 物化的节点（实际类型为 TextNode）。
/// @return 物化时解析后的文本。
static QString nodeText(const std::unique_ptr<BroadItem::Node>& n)
{
    return static_cast<const BroadItem::TextNode&>(*n).text;
}

/// @brief 经实例节点派发测量：node->element->measure(ctx, constraints, *node)。
/// @param n 实例节点。
/// @param ctx 布局上下文。
/// @param constraints 测量约束。
/// @return 测量结果。
static BroadItem::MeasureResult measureNode(const std::unique_ptr<BroadItem::Node>& n,
                                            const BroadItem::LayoutContext& ctx,
                                            const BroadItem::LayoutConstraints& constraints)
{
    return n->element->measure(ctx, constraints, *n);
}

// ============================================================
// 测试类
// ============================================================
class TestForElement : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    /// @brief testStringListIteration: QStringList 迭代，3 个元素
    void testStringListIteration()
    {
        // 准备数据 context: "names" = QStringList{"Alice","Bob","Carol"}
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("names", QStringList{"Alice", "Bob", "Carol"});

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: b:of="names" b:as="n", 模板绑定 b:content="n"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("names");
        forEl->setAsVariable("n");
        forEl->setTemplate(makeBoundText("n"));

        // 执行 materializeChildren()
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：3 个节点，每个的文本绑定到对应名字
        QCOMPARE(nodes.size(), size_t(3));
        QStringList expected{"Alice", "Bob", "Carol"};
        for (int i = 0; i < 3; ++i)
            QCOMPARE(nodeText(nodes[i]), expected[i]);
    }

    /// @brief testVariantListIteration: QVariantList (map) 迭代，2 个元素 + 嵌套路径 + map 键平铺
    void testVariantListIteration()
    {
        // 准备数据 context: "people" = QVariantList of maps
        BroadItem::MapPropertyContext mapCtx;
        QVariantList people;
        QVariantMap p1;
        p1["name"] = "Alice";
        p1["age"] = 30;
        people.append(p1);
        QVariantMap p2;
        p2["name"] = "Bob";
        p2["age"] = 25;
        people.append(p2);
        mapCtx.setProperty("people", people);

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: b:of="people" b:as="p", 模板绑定 b:content="p.name"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("people");
        forEl->setAsVariable("p");
        forEl->setTemplate(makeBoundText("p.name"));

        // 执行 materializeChildren()
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：返回 2 个节点，每个通过 per-item context 绑定 "p" → 当前 map
        QCOMPARE(nodes.size(), size_t(2));
        QCOMPARE(nodeText(nodes[0]), QString("Alice"));
        QCOMPARE(nodeText(nodes[1]), QString("Bob"));

        // 附加验证：map 键平铺 —— 不带 "p." 前缀的 "name" 直接从 per-item context 的
        // 平铺键解析（ForElement::materializeChildren 会把 map 键平铺进 itemCtx）。
        auto flatFor = std::make_shared<BroadItem::ForElement>();
        flatFor->setBindProperty("people");
        flatFor->setAsVariable("p");
        flatFor->setTemplate(makeBoundText("name"));  // 无前缀 → 平铺键

        auto flatNodes = flatFor->materializeChildren(ctx);
        QCOMPARE(flatNodes.size(), size_t(2));
        QCOMPARE(nodeText(flatNodes[0]), QString("Alice"));
        QCOMPARE(nodeText(flatNodes[1]), QString("Bob"));
    }

    /// @brief testNullPathWarning: 嵌套路径中缺键 → 该项解析失败并静默回退为空文本
    void testNullPathWarning()
    {
        // 准备数据: "users" = QVariantList, 第2个元素没有 address 键
        BroadItem::MapPropertyContext mapCtx;
        QVariantList users;
        QVariantMap u1;
        u1["address"] = QVariantMap{{"city", "NYC"}};
        users.append(u1);
        QVariantMap u2;
        u2["name"] = "Bob";  // 没有 address 键
        users.append(u2);
        mapCtx.setProperty("users", users);

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: b:of="users" b:as="u", 模板 b:content="u.address.city"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("users");
        forEl->setAsVariable("u");
        forEl->setTemplate(makeBoundText("u.address.city"));

        // 执行 materializeChildren()
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：返回 2 个节点；第2个节点的 "u.address.city" 解析失败。
        // 新架构中 TextElement::resolveText 先经 hasProperty 短路：缺键时不再产生
        // qCritical，而是静默回退为空文本（标签无静态文本）。
        QCOMPARE(nodes.size(), size_t(2));
        QCOMPARE(nodeText(nodes[0]), QString("NYC"));
        QVERIFY(nodeText(nodes[1]).isEmpty());
    }

    /// @brief testGlobalFallback: 非 b:as 前缀属性回退到全局 context
    void testGlobalFallback()
    {
        // 准备数据: 全局有 "title" = "Hello"; 每项有 {name: ...}
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("title", "Hello");

        QVariantList items;
        QVariantMap item1;
        item1["name"] = "Alice";
        items.append(item1);
        QVariantMap item2;
        item2["name"] = "Bob";
        items.append(item2);
        mapCtx.setProperty("items", items);

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: b:of="items" b:as="item"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");
        forEl->setAsVariable("item");

        // 模板绑定 "title" — 应从全局 context fallback 解析
        forEl->setTemplate(makeBoundText("title"));

        // 执行 materializeChildren()
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：2 个节点，"title" 从全局 context 解析为 "Hello"
        QCOMPARE(nodes.size(), size_t(2));
        for (int i = 0; i < 2; ++i)
            QCOMPARE(nodeText(nodes[i]), QString("Hello"));
    }

    /// @brief testBindsPropertyStripsAs: bindsProperty 应剥离 b:as 前缀
    void testBindsPropertyStripsAs()
    {
        // 构造 ForElement: b:of="users" b:as="u"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("users");
        forEl->setAsVariable("u");

        // 模板绑定 "name"（不带 "u." 前缀 — 纯全局绑定）
        forEl->setTemplate(makeBoundText("name"));

        // b:as="u" 剥离逻辑：per-item 变量和路径不触发全局 relayout
        QVERIFY(forEl->bindsProperty("users"));      // m_ofProperty 匹配
        QVERIFY(forEl->bindsProperty("name"));       // 模板绑定 "name"（无 as 前缀）
        QVERIFY(!forEl->bindsProperty("u"));         // b:as 变量本身不是全局属性
        QVERIFY(!forEl->bindsProperty("u.name"));    // per-item 路径，不是全局属性
    }

    /// @brief testEmptyListZeroElements: 空列表 → materializeChildren() 返回 0 个节点
    void testEmptyListZeroElements()
    {
        // 准备数据: "empty" = 空 QStringList
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("empty", QStringList{});

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: b:of="empty" b:as="e"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("empty");
        forEl->setAsVariable("e");
        forEl->setTemplate(makeBoundText("e"));

        // 执行 materializeChildren()
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：空列表 → 返回空 vector
        QCOMPARE(nodes.size(), size_t(0));
    }

    /// @brief testMissingAsError: 缺少 b:as 属性时 qCritical + 返回空
    void testMissingAsError()
    {
        // 准备数据 context
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"a", "b"});

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: b:of="items" 但 **没有** b:as
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");
        forEl->setTemplate(makeBoundText("x"));

        // 期望：缺少 b:as 时 qCritical 并返回空
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(".*b:as.*required.*"));

        // 执行 materializeChildren()
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：缺少 b:as → 返回空 vector
        QCOMPARE(nodes.size(), size_t(0));
    }

    /// @brief testStepAttributeRejected: step 属性被拒绝 + qWarning
    void testStepAttributeRejected()
    {
        // 准备数据 context
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"a", "b", "c", "d"});

        BroadItem::LayoutContext ctx{&mapCtx};

        // 通过 XML 解析构造 ForElement，含 step 属性
        QDomDocument doc;
        QDomElement el = doc.createElement("for");
        el.setAttributeNS(BroadItem::BINDING_NS, "b:of", "items");
        el.setAttributeNS(BroadItem::BINDING_NS, "b:as", "x");
        el.setAttribute("step", "2");

        auto forEl = std::make_shared<BroadItem::ForElement>();

        // 期望：解析时 step 未知属性被 warn
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*step.*"));

        forEl->parse(el);
        forEl->setTemplate(makeBoundText("x"));

        // 执行 materializeChildren() — step 属性被忽略，默认遍历全部 4 个元素
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：step 被忽略 → 遍历所有 4 个元素
        QCOMPARE(nodes.size(), size_t(4));
        QStringList expected{"a", "b", "c", "d"};
        for (int i = 0; i < 4; ++i)
            QCOMPARE(nodeText(nodes[i]), expected[i]);
    }

    /// @brief testNullItemSkipped: QVariantList 中 null 项被跳过 + qWarning
    void testNullItemSkipped()
    {
        // 准备数据: "items" = QVariantList 含一个 null 项
        BroadItem::MapPropertyContext mapCtx;
        QVariantList list;
        QVariantMap m1;
        m1["name"] = "Alice";
        list.append(m1);
        list.append(QVariant());   // null 项
        QVariantMap m3;
        m3["name"] = "Carol";
        list.append(m3);
        mapCtx.setProperty("items", list);

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: b:of="items" b:as="item"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");
        forEl->setAsVariable("item");
        forEl->setTemplate(makeBoundText("item.name"));

        // 期望：跳过 null 项时 qWarning
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*skipping null item.*"));

        // 执行 materializeChildren()
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：null 项被跳过 → 只返回 2 个有效节点
        QCOMPARE(nodes.size(), size_t(2));
        QCOMPARE(nodeText(nodes[0]), QString("Alice"));
        QCOMPARE(nodeText(nodes[1]), QString("Carol"));
    }

    /// @brief testDeepNestedPath: 深层嵌套路径 item.address.city
    void testDeepNestedPath()
    {
        // 准备数据: "people" = QVariantList, 每项有嵌套 address.city
        BroadItem::MapPropertyContext mapCtx;
        QVariantList people;
        QVariantMap p1;
        p1["address"] = QVariantMap{{"city", "NYC"}};
        people.append(p1);
        QVariantMap p2;
        p2["address"] = QVariantMap{{"city", "Tokyo"}};
        people.append(p2);
        mapCtx.setProperty("people", people);

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: b:of="people" b:as="item"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("people");
        forEl->setAsVariable("item");
        forEl->setTemplate(makeBoundText("item.address.city"));  // 深层嵌套路径

        // 执行 materializeChildren()
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：2 个节点，每个的 "item.address.city" 解析为对应城市
        QCOMPARE(nodes.size(), size_t(2));
        QCOMPARE(nodeText(nodes[0]), QString("NYC"));
        QCOMPARE(nodeText(nodes[1]), QString("Tokyo"));
    }

    /// @brief testMixedScalarVariantList: 标量值 QVariantList 元素迭代
    void testMixedScalarVariantList()
    {
        // 准备数据: "data" = QVariantList of scalar values (非 map)
        BroadItem::MapPropertyContext mapCtx;
        QVariantList data;
        data.append(QString("hello"));
        data.append(42);
        data.append(QString("world"));
        mapCtx.setProperty("data", data);

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: b:of="data" b:as="x"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("data");
        forEl->setAsVariable("x");
        forEl->setTemplate(makeBoundText("x"));  // 绑定标量 asVariable

        // 执行 materializeChildren()
        auto nodes = forEl->materializeChildren(ctx);

        // 验证：3 个标量元素全部迭代；标量值经 QVariant::toString 解析为文本
        QCOMPARE(nodes.size(), size_t(3));
        QCOMPARE(nodeText(nodes[0]), QString("hello"));
        QCOMPARE(nodeText(nodes[1]), QString("42"));   // int 42 → "42"
        QCOMPARE(nodeText(nodes[2]), QString("world"));
    }

    /// @brief testRowLayoutResolvesBindingsInFor: ForElement+RowLayout → TextNodes resolve per-item data
    void testRowLayoutResolvesBindingsInFor()
    {
        BroadItem::MapPropertyContext mapCtx;
        QVariantList processes;
        QVariantMap p1; p1["name"] = QString("nginx"); p1["pid"] = QString("1234"); p1["cpu"] = QString("2.1%");
        QVariantMap p2; p2["name"] = QString("redis"); p2["pid"] = QString("5678"); p2["cpu"] = QString("1.3%");
        QVariantMap p3; p3["name"] = QString("mysql"); p3["pid"] = QString("9012"); p3["cpu"] = QString("5.7%");
        processes.append(p1); processes.append(p2); processes.append(p3);
        mapCtx.setProperty("processes", processes);
        BroadItem::LayoutContext ctx{&mapCtx};

        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("processes");
        forEl->setAsVariable("p");

        auto row = std::make_shared<BroadItem::RowLayout>();
        row->addChild(makeBoundText("p.name", "11"));
        row->addChild(makeBoundText("p.pid", "10"));
        row->addChild(makeBoundText("p.cpu", "11"));
        forEl->setTemplate(row);

        auto nodes = forEl->materializeChildren(ctx);
        QCOMPARE(nodes.size(), size_t(3));

        QStringList expectedNames{"nginx", "redis", "mysql"};
        QStringList expectedPids{"1234", "5678", "9012"};
        QStringList expectedCpus{"2.1%", "1.3%", "5.7%"};
        for (int i = 0; i < 3; ++i) {
            // 物化节点的模板应为 RowLayout
            QVERIFY(dynamic_cast<const BroadItem::RowLayout*>(nodes[i]->element) != nullptr);
            // 触发行布局测量，验证文本解析已级联
            auto mr = measureNode(nodes[i], ctx, {500, 500});
            QVERIFY(mr.intrinsicSize.width() > 10);
            // 物化后的子节点（TextNode）验证逐项文本与各自测量
            const auto& children = nodes[i]->children;
            QCOMPARE(children.size(), size_t(3));
            QCOMPARE(nodeText(children[0]), expectedNames[i]);
            QCOMPARE(nodeText(children[1]), expectedPids[i]);
            QCOMPARE(nodeText(children[2]), expectedCpus[i]);
            for (size_t j = 0; j < 3; ++j) {
                auto childMr = measureNode(children[j], ctx, {500, 500});
                QVERIFY(childMr.intrinsicSize.width() > 1);
            }
        }
    }

    /// @brief testColumnLayoutResolvesBindingsInFor: ForElement+ColumnLayout → TextNodes resolve per-item data
    void testColumnLayoutResolvesBindingsInFor()
    {
        BroadItem::MapPropertyContext mapCtx;
        QVariantList items;
        QVariantMap m1; m1["label"] = QString("CPU"); m1["value"] = QString("45%");
        QVariantMap m2; m2["label"] = QString("MEM"); m2["value"] = QString("8.2G");
        items.append(m1); items.append(m2);
        mapCtx.setProperty("items", items);
        BroadItem::LayoutContext ctx{&mapCtx};

        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");
        forEl->setAsVariable("p");

        auto col = std::make_shared<BroadItem::ColumnLayout>();
        col->addChild(makeBoundText("p.label", "11"));
        col->addChild(makeBoundText("p.value", "11"));
        forEl->setTemplate(col);

        auto nodes = forEl->materializeChildren(ctx);
        QCOMPARE(nodes.size(), size_t(2));

        QStringList expectedLabels{"CPU", "MEM"};
        QStringList expectedValues{"45%", "8.2G"};
        for (int i = 0; i < 2; ++i) {
            QVERIFY(dynamic_cast<const BroadItem::ColumnLayout*>(nodes[i]->element) != nullptr);
            auto mr = measureNode(nodes[i], ctx, {500, 500});
            QVERIFY(mr.intrinsicSize.width() > 10);
            const auto& children = nodes[i]->children;
            QCOMPARE(children.size(), size_t(2));
            QCOMPARE(nodeText(children[0]), expectedLabels[i]);
            QCOMPARE(nodeText(children[1]), expectedValues[i]);
            QVERIFY(measureNode(children[0], ctx, {500, 500}).intrinsicSize.width() > 1);
            QVERIFY(measureNode(children[1], ctx, {500, 500}).intrinsicSize.width() > 1);
        }
    }

    /// @brief testIfHasResolvesBindingsInClonedChild: positive + negative cases for IfHasElement with TextElement
    void testIfHasResolvesBindingsInClonedChild()
    {
        // Positive: warning property exists → TextNode resolves "System alert"
        {
            BroadItem::MapPropertyContext mapCtx;
            mapCtx.setProperty("warning", QString("System alert"));
            BroadItem::LayoutContext ctx{&mapCtx};

            auto ifEl = std::make_shared<BroadItem::IfHasElement>();
            ifEl->setBindProperty("warning");
            ifEl->setChild(makeBoundText("warning", "11"));

            auto nodes = ifEl->materializeChildren(ctx);
            QCOMPARE(nodes.size(), size_t(1));
            QCOMPARE(nodeText(nodes[0]), QString("System alert"));
            auto mr = measureNode(nodes[0], ctx, {500, 500});
            QVERIFY(mr.intrinsicSize.width() > 10);
        }

        // Negative: no warning property → materializeChildren returns empty
        {
            BroadItem::MapPropertyContext mapCtx;
            BroadItem::LayoutContext ctx{&mapCtx};

            auto ifEl = std::make_shared<BroadItem::IfHasElement>();
            ifEl->setBindProperty("warning");
            ifEl->setChild(makeBoundText("warning", "11"));

            auto nodes = ifEl->materializeChildren(ctx);
            QCOMPARE(nodes.size(), size_t(0));
        }
    }

    /// @brief testNestedContainerResolvesBindings: GridLayout→ColumnLayout→TextElement in ForElement (covers GridLayout)
    void testNestedContainerResolvesBindings()
    {
        BroadItem::MapPropertyContext mapCtx;
        QVariantList items;
        QVariantMap p1; p1["name"] = QString("Alice");
        QVariantMap p2; p2["name"] = QString("Bob");
        items.append(p1); items.append(p2);
        mapCtx.setProperty("items", items);
        BroadItem::LayoutContext ctx{&mapCtx};

        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");
        forEl->setAsVariable("p");

        // GridLayout → ColumnLayout → TextElement "p.name"
        auto grid = std::make_shared<BroadItem::GridLayout>();
        auto innerCol = std::make_shared<BroadItem::ColumnLayout>();
        innerCol->addChild(makeBoundText("p.name", "11"));
        grid->addChild(innerCol);
        forEl->setTemplate(grid);

        auto nodes = forEl->materializeChildren(ctx);
        QCOMPARE(nodes.size(), size_t(2));

        QStringList expectedNames{"Alice", "Bob"};
        for (int i = 0; i < 2; ++i) {
            QVERIFY(dynamic_cast<const BroadItem::GridLayout*>(nodes[i]->element) != nullptr);
            auto mr = measureNode(nodes[i], ctx, {500, 500});
            QVERIFY(mr.intrinsicSize.width() > 10);
            // 实例节点的物化子节点：GridLayout 下应只有 1 个 ColumnLayout 节点
            const auto& gridChildren = nodes[i]->children;
            QCOMPARE(gridChildren.size(), size_t(1));
            QVERIFY(dynamic_cast<const BroadItem::ColumnLayout*>(gridChildren[0]->element) != nullptr);
            auto colMr = measureNode(gridChildren[0], ctx, {500, 500});
            QVERIFY(colMr.intrinsicSize.width() > 1);
            // 验证 depth=2 的文本已解析
            const auto& colChildren = gridChildren[0]->children;
            QCOMPARE(colChildren.size(), size_t(1));
            QCOMPARE(nodeText(colChildren[0]), expectedNames[i]);
        }
    }

    /// @brief testGlobalBindingsStillWorkInContainers: plain RowLayout without ForElement resolves global properties
    void testGlobalBindingsStillWorkInContainers()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("cpu", QString("45%"));
        BroadItem::LayoutContext ctx{&mapCtx};

        auto row = std::make_shared<BroadItem::RowLayout>();
        row->addChild(makeBoundText("cpu", "11"));

        // 物化：文本在物化时从全局 context 解析
        auto node = row->materialize(ctx);
        QVERIFY(node != nullptr);
        QCOMPARE(node->children.size(), size_t(1));
        QCOMPARE(nodeText(node->children[0]), QString("45%"));

        // Verify text resolved: measure must return non-zero width
        auto mr = row->measure(ctx, {500, 500}, *node);
        QVERIFY2(mr.intrinsicSize.width() > 10,
                 qPrintable(QString("Expected non-zero width for resolved '45%', got %1")
                            .arg(mr.intrinsicSize.width())));
    }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestForElement)
#include "test_for_element.moc"
