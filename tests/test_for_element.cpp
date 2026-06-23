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
#include <QDebug>

// Helper to create TextElement with b:content binding programmatically
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

// ============================================================
// 测试用 Mock 元素 — 用作 ForElement 的模板
// ============================================================
class ForTestElement : public BroadItem::Element {
public:
    explicit ForTestElement() = default;

    BroadItem::MeasureResult measure(const BroadItem::LayoutContext&,
                                     const BroadItem::LayoutConstraints&) override
    {
        return {QSizeF(100, 20)};
    }
    void layout(const BroadItem::LayoutContext&, const QRectF& rect) override { m_rect = rect; }
    void render(QPainter*, const BroadItem::LayoutContext&) const override {}

    BroadItem::ElementPtr clone() const override
    {
        auto c = std::make_shared<ForTestElement>();
        c->m_boundNames = m_boundNames;
        return c;
    }

    bool bindsProperty(const QString& name) const override
    {
        return m_boundNames.contains(name);
    }

    /// @brief 覆写 resolveBindings 以从 context 解析所有绑定属性
    void resolveBindings(const BroadItem::LayoutContext& ctx) override
    {
        m_resolvedValues.clear();
        for (const auto& name : m_boundNames) {
            m_resolvedValues[name] = ctx.property(name);
        }
    }

    /// @brief 模拟此元素绑定到的属性名集合。
    QSet<QString> m_boundNames;
    /// @brief resolveBindings() 后每个绑定属性的解析值。
    QHash<QString, QVariant> m_resolvedValues;
};

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

        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("n");  // 模板绑定变量 "n" (as)
        forEl->setTemplate(tmpl);

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：3 个元素，每个 "n" 绑定到对应名字
        QCOMPARE(result.size(), 3);
        QStringList expected{"Alice", "Bob", "Carol"};
        for (int i = 0; i < 3; ++i) {
            auto* clone = dynamic_cast<ForTestElement*>(result[i].get());
            QVERIFY(clone != nullptr);
            QCOMPARE(clone->m_resolvedValues["n"].toString(), expected[i]);
        }
    }

    /// @brief testVariantListIteration: QVariantList (map) 迭代，2 个元素 + 嵌套路径
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

        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("p.name");  // 模板绑定 "p.name"
        forEl->setTemplate(tmpl);

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：返回 2 个元素，每个通过 per-item context 绑定 "p" → 当前 map
        QCOMPARE(result.size(), 2);

        auto* clone0 = dynamic_cast<ForTestElement*>(result[0].get());
        QVERIFY(clone0 != nullptr);
        QCOMPARE(clone0->m_resolvedValues["p.name"].toString(), QString("Alice"));

        auto* clone1 = dynamic_cast<ForTestElement*>(result[1].get());
        QVERIFY(clone1 != nullptr);
        QCOMPARE(clone1->m_resolvedValues["p.name"].toString(), QString("Bob"));
    }

    /// @brief testNullPathWarning: 嵌套路径中缺键 → qCritical + 解析返回无效值
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

        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("u.address.city");
        forEl->setTemplate(tmpl);

        // 期望 qt 消息：address 不在 map 中 → MapPropertyContext 报告 not found
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(".*address.*not found.*"));

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：返回 2 个元素；第2个元素的 "u.address.city" 解析为无效 QVariant
        QCOMPARE(result.size(), 2);

        auto* clone0 = dynamic_cast<ForTestElement*>(result[0].get());
        QVERIFY(clone0 != nullptr);
        QCOMPARE(clone0->m_resolvedValues["u.address.city"].toString(), QString("NYC"));

        auto* clone1 = dynamic_cast<ForTestElement*>(result[1].get());
        QVERIFY(clone1 != nullptr);
        QVERIFY(!clone1->m_resolvedValues["u.address.city"].isValid());
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

        auto tmpl = std::make_shared<ForTestElement>();
        // 模板绑定 "title" — 应从全局 context fallback 解析
        tmpl->m_boundNames.insert("title");
        forEl->setTemplate(tmpl);

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：2 个元素，"title" 从全局 context 解析为 "Hello"
        QCOMPARE(result.size(), 2);
        for (int i = 0; i < 2; ++i) {
            auto* clone = dynamic_cast<ForTestElement*>(result[i].get());
            QVERIFY(clone != nullptr);
            QCOMPARE(clone->m_resolvedValues["title"].toString(), QString("Hello"));
        }
    }

    /// @brief testBindsPropertyStripsAs: bindsProperty 应剥离 b:as 前缀
    void testBindsPropertyStripsAs()
    {
        // 构造 ForElement: b:of="users" b:as="u"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("users");
        forEl->setAsVariable("u");

        // 模板绑定 "name"（不带 "u." 前缀 — 纯全局绑定）
        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("name");
        forEl->setTemplate(tmpl);

        // b:as="u" 剥离逻辑：per-item 变量和路径不触发全局 relayout
        QVERIFY(forEl->bindsProperty("users"));      // m_ofProperty 匹配
        QVERIFY(forEl->bindsProperty("name"));       // 模板绑定 "name"（无 as 前缀）
        QVERIFY(!forEl->bindsProperty("u"));         // b:as 变量本身不是全局属性
        QVERIFY(!forEl->bindsProperty("u.name"));    // per-item 路径，不是全局属性
    }

    /// @brief testEmptyListZeroElements: 空列表 → expand() 返回 0 个元素
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

        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("e");
        forEl->setTemplate(tmpl);

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：空列表 → 返回空 vector
        QCOMPARE(result.size(), 0);
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

        auto tmpl = std::make_shared<ForTestElement>();
        forEl->setTemplate(tmpl);

        // 期望：缺少 b:as 时 qCritical 并返回空
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(".*b:as.*required.*"));

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：缺少 b:as → 返回空 vector
        QCOMPARE(result.size(), 0);
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

        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("x");
        forEl->setTemplate(tmpl);

        // 执行 expand() — step 属性被忽略，默认遍历全部 4 个元素
        auto result = forEl->expand(ctx);

        // 验证：step 被忽略 → 遍历所有 4 个元素
        QCOMPARE(result.size(), 4);
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

        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("item.name");
        forEl->setTemplate(tmpl);

        // 期望：跳过 null 项时 qWarning
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*skipping null item.*"));

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：null 项被跳过 → 只返回 2 个有效元素
        QCOMPARE(result.size(), 2);

        auto* clone0 = dynamic_cast<ForTestElement*>(result[0].get());
        QVERIFY(clone0 != nullptr);
        QCOMPARE(clone0->m_resolvedValues["item.name"].toString(), QString("Alice"));

        auto* clone1 = dynamic_cast<ForTestElement*>(result[1].get());
        QVERIFY(clone1 != nullptr);
        QCOMPARE(clone1->m_resolvedValues["item.name"].toString(), QString("Carol"));
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

        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("item.address.city");  // 深层嵌套路径
        forEl->setTemplate(tmpl);

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：2 个元素，每个的 "item.address.city" 解析为对应城市
        QCOMPARE(result.size(), 2);

        auto* clone0 = dynamic_cast<ForTestElement*>(result[0].get());
        QVERIFY(clone0 != nullptr);
        QCOMPARE(clone0->m_resolvedValues["item.address.city"].toString(), QString("NYC"));

        auto* clone1 = dynamic_cast<ForTestElement*>(result[1].get());
        QVERIFY(clone1 != nullptr);
        QCOMPARE(clone1->m_resolvedValues["item.address.city"].toString(), QString("Tokyo"));
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

        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("x");  // 绑定标量 asVariable
        forEl->setTemplate(tmpl);

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：3 个标量元素全部迭代
        QCOMPARE(result.size(), 3);

        auto* clone0 = dynamic_cast<ForTestElement*>(result[0].get());
        QVERIFY(clone0 != nullptr);
        QCOMPARE(clone0->m_resolvedValues["x"].toString(), QString("hello"));

        auto* clone1 = dynamic_cast<ForTestElement*>(result[1].get());
        QVERIFY(clone1 != nullptr);
        QCOMPARE(clone1->m_resolvedValues["x"].toInt(), 42);

        auto* clone2 = dynamic_cast<ForTestElement*>(result[2].get());
        QVERIFY(clone2 != nullptr);
        QCOMPARE(clone2->m_resolvedValues["x"].toString(), QString("world"));
    }

    /// @brief testRowLayoutResolvesBindingsInFor: ForElement+RowLayout → TextElements resolve per-item data
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

        auto result = forEl->expand(ctx);
        QCOMPARE(result.size(), 3);

        QStringList expectedNames{"nginx", "redis", "mysql"};
        QStringList expectedPids{"1234", "5678", "9012"};
        QStringList expectedCpus{"2.1%", "1.3%", "5.7%"};
        for (int i = 0; i < 3; ++i) {
            auto* cloneRow = dynamic_cast<BroadItem::RowLayout*>(result[i].get());
            QVERIFY(cloneRow != nullptr);
            // Trigger flatten + measure to verify text resolution cascaded
            auto mr = cloneRow->measure(ctx, {500, 500});
            QVERIFY(mr.intrinsicSize.width() > 10);
            // Access flattened children to verify individual TextElement measurements
            const auto& children = cloneRow->flattenedChildren();
            QCOMPARE(children.size(), 3);
            for (size_t j = 0; j < 3; ++j) {
                auto childMr = children[j]->measure(ctx, {500, 500});
                QVERIFY(childMr.intrinsicSize.width() > 1);
            }
        }
    }

    /// @brief testColumnLayoutResolvesBindingsInFor: ForElement+ColumnLayout → TextElements resolve per-item data
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

        auto result = forEl->expand(ctx);
        QCOMPARE(result.size(), 2);

        for (int i = 0; i < 2; ++i) {
            auto* cloneCol = dynamic_cast<BroadItem::ColumnLayout*>(result[i].get());
            QVERIFY(cloneCol != nullptr);
            auto mr = cloneCol->measure(ctx, {500, 500});
            QVERIFY(mr.intrinsicSize.width() > 10);
            const auto& children = cloneCol->flattenedChildren();
            QCOMPARE(children.size(), 2);
            QVERIFY(children[0]->measure(ctx, {500, 500}).intrinsicSize.width() > 1);
            QVERIFY(children[1]->measure(ctx, {500, 500}).intrinsicSize.width() > 1);
        }
    }

    /// @brief testIfHasResolvesBindingsInClonedChild: positive + negative cases for IfHasElement with TextElement
    void testIfHasResolvesBindingsInClonedChild()
    {
        // Positive: warning property exists → TextElement resolves "System alert"
        {
            BroadItem::MapPropertyContext mapCtx;
            mapCtx.setProperty("warning", QString("System alert"));
            BroadItem::LayoutContext ctx{&mapCtx};

            auto ifEl = std::make_shared<BroadItem::IfHasElement>();
            ifEl->setBindProperty("warning");
            ifEl->setChild(makeBoundText("warning", "11"));

            auto result = ifEl->expand(ctx);
            QCOMPARE(result.size(), 1);
            auto mr = result[0]->measure(ctx, {500, 500});
            QVERIFY(mr.intrinsicSize.width() > 10);
        }

        // Negative: no warning property → expand returns empty
        {
            BroadItem::MapPropertyContext mapCtx;
            BroadItem::LayoutContext ctx{&mapCtx};

            auto ifEl = std::make_shared<BroadItem::IfHasElement>();
            ifEl->setBindProperty("warning");
            ifEl->setChild(makeBoundText("warning", "11"));

            auto result = ifEl->expand(ctx);
            QCOMPARE(result.size(), 0);
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

        auto result = forEl->expand(ctx);
        QCOMPARE(result.size(), 2);

        for (int i = 0; i < 2; ++i) {
            auto* cloneGrid = dynamic_cast<BroadItem::GridLayout*>(result[i].get());
            QVERIFY(cloneGrid != nullptr);
            auto mr = cloneGrid->measure(ctx, {500, 500});
            QVERIFY(mr.intrinsicSize.width() > 10);
            // GridLayout exposes children() publicly; verify depth=2 resolved
            const auto& gridChildren = cloneGrid->children();
            QCOMPARE(gridChildren.size(), 1);
            auto* nestedCol = dynamic_cast<BroadItem::ColumnLayout*>(gridChildren[0].get());
            QVERIFY(nestedCol != nullptr);
            auto colMr = nestedCol->measure(ctx, {500, 500});
            QVERIFY(colMr.intrinsicSize.width() > 1);
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

        // Call ContainerElement::resolveBindings directly on the RowLayout
        row->resolveBindings(ctx);

        // Verify text resolved: measure must return non-zero width
        auto mr = row->measure(ctx, {500, 500});
        QVERIFY2(mr.intrinsicSize.width() > 10,
                 qPrintable(QString("Expected non-zero width for resolved '45%', got %1")
                            .arg(mr.intrinsicSize.width())));
    }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestForElement)
#include "test_for_element.moc"
