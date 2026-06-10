#include <QtTest/QtTest>
#include "broaditem/elements/ForElement.h"
#include "broaditem/LayoutContext.h"
#include "broaditem/MapPropertyContext.h"
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>

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

        // 构造 ForElement: :of="names" :as="n", 模板绑定 :content="n"
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

        // 构造 ForElement: :of="people" :as="p", 模板绑定 :content="p.name"
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

        // 构造 ForElement: :of="users" :as="u", 模板 :content="u.address.city"
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

    /// @brief testGlobalFallback: 非 :as 前缀属性回退到全局 context
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

        // 构造 ForElement: :of="items" :as="item"
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

    /// @brief testBindsPropertyStripsAs: bindsProperty 应剥离 :as 前缀
    void testBindsPropertyStripsAs()
    {
        // 构造 ForElement: :of="users" :as="u"
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("users");
        forEl->setAsVariable("u");

        // 模板绑定 "name"（不带 "u." 前缀 — 纯全局绑定）
        auto tmpl = std::make_shared<ForTestElement>();
        tmpl->m_boundNames.insert("name");
        forEl->setTemplate(tmpl);

        // :as="u" 剥离逻辑：per-item 变量和路径不触发全局 relayout
        QVERIFY(forEl->bindsProperty("users"));      // m_ofProperty 匹配
        QVERIFY(forEl->bindsProperty("name"));       // 模板绑定 "name"（无 as 前缀）
        QVERIFY(!forEl->bindsProperty("u"));         // :as 变量本身不是全局属性
        QVERIFY(!forEl->bindsProperty("u.name"));    // per-item 路径，不是全局属性
    }

    /// @brief testEmptyListZeroElements: 空列表 → expand() 返回 0 个元素
    void testEmptyListZeroElements()
    {
        // 准备数据: "empty" = 空 QStringList
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("empty", QStringList{});

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: :of="empty" :as="e"
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

    /// @brief testMissingAsError: 缺少 :as 属性时 qCritical + 返回空
    void testMissingAsError()
    {
        // 准备数据 context
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"a", "b"});

        BroadItem::LayoutContext ctx{&mapCtx};

        // 构造 ForElement: :of="items" 但 **没有** :as
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");

        auto tmpl = std::make_shared<ForTestElement>();
        forEl->setTemplate(tmpl);

        // 期望：缺少 :as 时 qCritical 并返回空
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(".*:as.*required.*"));

        // 执行 expand()
        auto result = forEl->expand(ctx);

        // 验证：缺少 :as → 返回空 vector
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
        el.setAttribute(":of", "items");
        el.setAttribute(":as", "x");
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

        // 构造 ForElement: :of="items" :as="item"
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

        // 构造 ForElement: :of="people" :as="item"
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

        // 构造 ForElement: :of="data" :as="x"
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

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestForElement)
#include "test_for_element.moc"
