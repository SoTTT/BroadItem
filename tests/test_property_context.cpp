#include <QtTest/QtTest>
#include "broaditem/PropertyContext.h"
#include "broaditem/MapPropertyContext.h"
#include "broaditem/QPropertyContext.h"
#include "broaditem/BroadItem.h"
#include <QFile>
#include <QDebug>

// ============================================================
// 测试用 QPropertyContext 子类
// ============================================================
class TestQProps : public BroadItem::QPropertyContext {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QStringList tags READ tags WRITE setTags NOTIFY tagsChanged)
    Q_PROPERTY(QVariantMap device READ device WRITE setDevice NOTIFY deviceChanged)
    Q_PROPERTY(QVariantList items READ items WRITE setItems NOTIFY itemsChanged)
public:
    [[nodiscard]] QString name() const { return m_name; }
    void setName(const QString& v) {
        if (m_name != v) { m_name = v; emit nameChanged(); }
    }
    [[nodiscard]] QStringList tags() const { return m_tags; }
    void setTags(const QStringList& v) {
        if (m_tags != v) { m_tags = v; emit tagsChanged(); }
    }
    [[nodiscard]] QVariantMap device() const { return m_device; }
    void setDevice(const QVariantMap& v) {
        if (m_device != v) { m_device = v; emit deviceChanged(); }
    }
    [[nodiscard]] QVariantList items() const { return m_items; }
    void setItems(const QVariantList& v) {
        if (m_items != v) { m_items = v; emit itemsChanged(); }
    }

signals:
    void nameChanged();
    void tagsChanged();
    void deviceChanged();
    void itemsChanged();

private:
    QString m_name;
    QStringList m_tags;
    QVariantMap m_device;
    QVariantList m_items;
};

// ============================================================
// 测试用 BroadItem 子类（带 Q_PROPERTY，用于 ItemPropertyContext）
// ============================================================
class TestPropItem : public BroadItem::BroadItem {
    Q_OBJECT
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString level READ level WRITE setLevel NOTIFY levelChanged)
    Q_PROPERTY(QVariantMap config READ config WRITE setConfig NOTIFY configChanged)
    Q_PROPERTY(QVariantList nodes READ nodes WRITE setNodes NOTIFY nodesChanged)
public:
    using BroadItem::BroadItem;

    [[nodiscard]] QString status() const { return m_status; }
    void setStatus(const QString& v) {
        if (m_status != v) { m_status = v; emit statusChanged(); }
    }
    [[nodiscard]] QString level() const { return m_level; }
    void setLevel(const QString& v) {
        if (m_level != v) { m_level = v; emit levelChanged(); }
    }
    [[nodiscard]] QVariantMap config() const { return m_config; }
    void setConfig(const QVariantMap& v) {
        if (m_config != v) { m_config = v; emit configChanged(); }
    }
    [[nodiscard]] QVariantList nodes() const { return m_nodes; }
    void setNodes(const QVariantList& v) {
        if (m_nodes != v) { m_nodes = v; emit nodesChanged(); }
    }

signals:
    void statusChanged();
    void levelChanged();
    void configChanged();
    void nodesChanged();

private:
    QString m_status;
    QString m_level;
    QVariantMap m_config;
    QVariantList m_nodes;
};

// ============================================================
// 测试类
// ============================================================
class TestPropertyContext : public QObject {
    Q_OBJECT
private:
    QString m_tempXmlPath;

private slots:
    void initTestCase()
    {
        QFile f("_test_item_layout.xml");
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write("<root><text>X</text></root>");
        f.close();
        m_tempXmlPath = f.fileName();
    }

    void cleanupTestCase()
    {
        QFile::remove(m_tempXmlPath);
    }

    // ==================== MapPropertyContext ====================
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void testMapSetGet()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        QCOMPARE(ctx.property("cpu").toString(), QString("45%"));
    }

    void testMapHasProperty()
    {
        BroadItem::MapPropertyContext ctx;
        QVERIFY(!ctx.hasProperty("cpu"));
        ctx.setProperty("cpu", "45%");
        QVERIFY(ctx.hasProperty("cpu"));
    }

    void testMapNullRemoves()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        QVERIFY(ctx.hasProperty("cpu"));

        ctx.setProperty("cpu", QVariant());
        QVERIFY(!ctx.hasProperty("cpu"));
        QVERIFY(!ctx.property("cpu").isValid());
    }

    void testMapNullRemovesInvalid()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        ctx.setProperty("cpu", QVariant());
        QVERIFY(!ctx.hasProperty("cpu"));
    }

    void testMapNoNotifyOnSame()
    {
        BroadItem::MapPropertyContext ctx;
        int count = 0;
        ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });

        ctx.setProperty("cpu", "45%");
        QCOMPARE(count, 1);

        ctx.setProperty("cpu", "45%");
        QCOMPARE(count, 1);
    }

    // ---------- 路径：扁平键 ----------

    void testMapPathFlat()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        QCOMPARE(ctx.property("cpu").toString(), QString("45%"));
        QVERIFY(ctx.hasProperty("cpu"));
    }

    void testMapPathFlatWithDotInName()
    {
        // 含 . 的 key 触发路径遍历，不会匹配顶层 map 中的这个键（因为段 0 不存在）
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("a.b", "val");
        // "a.b" 会先找顶层 "a"，如果 "a" 不存在 → 报错
        QVERIFY(!ctx.property("a.b").isValid());
    }

    // ---------- 路径：点号 ----------

    void testMapPathDot()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap device;
        device["cpu"] = "45%";
        device["mem"] = "60%";
        ctx.setProperty("device", device);

        QCOMPARE(ctx.property("device.cpu").toString(), QString("45%"));
        QCOMPARE(ctx.property("device.mem").toString(), QString("60%"));
    }

    void testMapPathDotDeep()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap inner;
        inner["value"] = "hello";
        QVariantMap middle;
        middle["inner"] = inner;
        ctx.setProperty("outer", middle);

        QCOMPARE(ctx.property("outer.inner.value").toString(), QString("hello"));
    }

    // ---------- 路径：下标 ----------

    void testMapPathBracket()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items.append("first");
        items.append("second");
        items.append("third");
        ctx.setProperty("items", items);

        QCOMPARE(ctx.property("items[0]").toString(), QString("first"));
        QCOMPARE(ctx.property("items[2]").toString(), QString("third"));
    }

    void testMapPathBracketNested()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap proc;
        proc["name"] = "nginx";
        proc["pid"] = "1234";
        QVariantList procs;
        procs.append(proc);
        ctx.setProperty("processes", procs);

        QCOMPARE(ctx.property("processes[0].name").toString(), QString("nginx"));
        QCOMPARE(ctx.property("processes[0].pid").toString(), QString("1234"));
    }

    void testMapPathMultiBracket()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList innerA;
        innerA.append("a0");
        innerA.append("a1");
        QVariantList innerB;
        innerB.append("b0");
        QVariantList outer;
        outer.append(QVariant(innerA));
        outer.append(QVariant(innerB));
        ctx.setProperty("grid", outer);

        QCOMPARE(ctx.property("grid[0][0]").toString(), QString("a0"));
        QCOMPARE(ctx.property("grid[0][1]").toString(), QString("a1"));
        QCOMPARE(ctx.property("grid[1][0]").toString(), QString("b0"));
    }

    // ---------- 类型错误 ----------

    void testMapPathKeyNotFound()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("device", QVariantMap{});
        QVERIFY(!ctx.property("device.unknown").isValid());
    }

    void testMapPathNotObject()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("val", "hello");
        // "val.anything" → val 是 QString，不是 map
        QVERIFY(!ctx.property("val.anything").isValid());
    }

    void testMapPathNotArray()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("val", "hello");
        // "val[0]" → val 是 QString，不是 list
        QVERIFY(!ctx.property("val[0]").isValid());
    }

    void testMapPathIndexOutOfBounds()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items.append("only");
        ctx.setProperty("items", items);

        QVERIFY(!ctx.property("items[3]").isValid());
    }

    // ---------- 语法错误 ----------

    void testMapPathSyntaxEmpty()
    {
        BroadItem::MapPropertyContext ctx;
        QVERIFY(!ctx.property("").isValid());
        QVERIFY(!ctx.hasProperty(""));
    }

    void testMapPathSyntaxEmptyKey()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap map;
        map["a"] = "val";
        ctx.setProperty("root", map);
        QVERIFY(!ctx.property("root..a").isValid());
    }

    void testMapPathSyntaxUnmatched()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items.append("x");
        ctx.setProperty("items", items);
        QVERIFY(!ctx.property("items[0").isValid());
    }

    void testMapPathSyntaxBadIndex()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items.append("x");
        ctx.setProperty("items", items);
        QVERIFY(!ctx.property("items[abc]").isValid());
    }

    void testMapPathSyntaxNegative()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items.append("x");
        ctx.setProperty("items", items);
        QVERIFY(!ctx.property("items[-1]").isValid());
    }

    void testMapPathSyntaxMissingDot()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap inner;
        inner["x"] = "y";
        QVariantList items;
        items.append(QVariant(inner));
        ctx.setProperty("items", items);
        QVERIFY(!ctx.property("items[0]x").isValid());
    }

    // ---------- hasProperty with paths ----------

    void testMapHasPathTrue()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap device;
        device["cpu"] = "45%";
        ctx.setProperty("device", device);

        QVERIFY(ctx.hasProperty("device.cpu"));
        QVERIFY(ctx.hasProperty("device"));
        QVERIFY(!ctx.hasProperty("device.ram"));
    }

    void testMapHasPathTopLevelMissing()
    {
        BroadItem::MapPropertyContext ctx;
        QVERIFY(!ctx.hasProperty("device.cpu"));
    }

    // ---------- 回调通知 ----------

    void testMapNotifyOnSet()
    {
        BroadItem::MapPropertyContext ctx;
        QString lastName;
        QVariant lastValue;
        ctx.setOnChanged([&](const QString& n, const QVariant& v) {
            lastName = n;
            lastValue = v;
        });

        ctx.setProperty("cpu", "45%");
        QCOMPARE(lastName, QString("cpu"));
        QCOMPARE(lastValue.toString(), QString("45%"));
    }

    void testMapNotifyOnNull()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");

        QString lastName;
        QVariant lastValue;
        bool called = false;
        ctx.setOnChanged([&](const QString& n, const QVariant& v) {
            lastName = n;
            lastValue = v;
            called = true;
        });

        ctx.setProperty("cpu", QVariant());
        QVERIFY(called);
        QCOMPARE(lastName, QString("cpu"));
        QVERIFY(!lastValue.isValid());
    }

    void testMapNotifyCount()
    {
        BroadItem::MapPropertyContext ctx;
        int count = 0;
        ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });

        ctx.setProperty("a", "1");
        ctx.setProperty("b", "2");
        ctx.setProperty("c", "3");
        QCOMPARE(count, 3);
    }

    void testMapRemoveNonexistentNoNotify()
    {
        BroadItem::MapPropertyContext ctx;
        int count = 0;
        ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });

        ctx.setProperty("cpu", QVariant());
        QCOMPARE(count, 0);  // 删不存在的 key 不通知
    }
    // NOLINTEND(readability-convert-member-functions-to-static)

    // ==================== MapPropertyContext 嵌套写入 ====================
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void testMapSetNestedDot()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        dev["mem"] = "60%";
        ctx.setProperty("device", dev);

        ctx.setProperty("device.cpu", "100%");
        QCOMPARE(ctx.property("device.cpu").toString(), QString("100%"));
        QCOMPARE(ctx.property("device.mem").toString(), QString("60%"));
    }

    void testMapSetNestedBracket()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items << "a" << "b" << "c";
        ctx.setProperty("items", items);

        ctx.setProperty("items[1]", "X");
        QCOMPARE(ctx.property("items[1]").toString(), QString("X"));
        QCOMPARE(ctx.property("items[0]").toString(), QString("a"));
        QCOMPARE(ctx.property("items[2]").toString(), QString("c"));
    }

    void testMapSetNestedBracketDot()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap inner;
        inner["name"] = "nginx";
        inner["pid"] = "1234";
        QVariantList items;
        items << inner;
        ctx.setProperty("processes", items);

        ctx.setProperty("processes[0].name", "httpd");
        QCOMPARE(ctx.property("processes[0].name").toString(), QString("httpd"));
        QCOMPARE(ctx.property("processes[0].pid").toString(), QString("1234"));
    }

    void testMapSetNestedDeep()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap inner;
        inner["value"] = "old";
        QVariantMap middle;
        middle["inner"] = inner;
        ctx.setProperty("outer", middle);

        ctx.setProperty("outer.inner.value", "new");
        QCOMPARE(ctx.property("outer.inner.value").toString(), QString("new"));
    }

    void testMapSetNestedMultiBracket()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList row0;
        row0 << "a0" << "a1";
        QVariantList row1;
        row1 << "b0";
        QVariantList grid;
        grid << QVariant(row0) << QVariant(row1);
        ctx.setProperty("grid", grid);

        ctx.setProperty("grid[0][1]", "X");
        QCOMPARE(ctx.property("grid[0][1]").toString(), QString("X"));
        QCOMPARE(ctx.property("grid[0][0]").toString(), QString("a0"));
    }

    void testMapSetNestedNotify()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);

        QString lastName;
        QVariant lastValue;
        ctx.setOnChanged([&](const QString& n, const QVariant& v) {
            lastName = n;
            lastValue = v;
        });

        ctx.setProperty("device.cpu", "100%");
        QCOMPARE(lastName, QString("device"));
        QVERIFY(lastValue.isValid());
    }

    // ---------- 嵌套写入：类型/存在性错误 ----------

    void testMapSetNestedMissingTopLevel()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("nonexistent.cpu", "val");
        // 首段不存在 → 不写入，不抛出
        QVERIFY(true);
    }

    void testMapSetNestedNotMap()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("str", "hello");
        ctx.setProperty("str.key", "val");
        // "str" 是字面量，不是 map → 不写入
        QCOMPARE(ctx.property("str").toString(), QString("hello"));
    }

    void testMapSetNestedNotList()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("str", "hello");
        ctx.setProperty("str[0]", "val");
        // "str" 不是 list → 不写入
        QCOMPARE(ctx.property("str").toString(), QString("hello"));
    }

    void testMapSetNestedIndexOOB()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items << "only";
        ctx.setProperty("items", items);
        ctx.setProperty("items[3]", "val");
        QCOMPARE(ctx.property("items[0]").toString(), QString("only"));
    }

    // ---------- 嵌套写入：语法错误 ----------

    void testMapSetNestedSyntaxDoubleDot()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap root;
        root["a"] = "val";
        ctx.setProperty("root", root);
        ctx.setProperty("root..a", "X");
        QCOMPARE(ctx.property("root.a").toString(), QString("val"));
    }

    void testMapSetNestedSyntaxTrailingDot()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap root;
        root["a"] = "val";
        ctx.setProperty("root", root);
        ctx.setProperty("root.", "X");
        QCOMPARE(ctx.property("root.a").toString(), QString("val"));
    }

    void testMapSetNestedSyntaxUnmatched()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items << "x";
        ctx.setProperty("items", items);
        ctx.setProperty("items[0", "X");
        QCOMPARE(ctx.property("items[0]").toString(), QString("x"));
    }

    void testMapSetNestedSyntaxBadIndex()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items << "x";
        ctx.setProperty("items", items);
        ctx.setProperty("items[abc]", "X");
        QCOMPARE(ctx.property("items[0]").toString(), QString("x"));
    }

    void testMapSetNestedSyntaxNegative()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items << "x";
        ctx.setProperty("items", items);
        ctx.setProperty("items[-1]", "X");
        QCOMPARE(ctx.property("items[0]").toString(), QString("x"));
    }

    void testMapSetNestedSyntaxNoDot()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap inner;
        inner["x"] = "y";
        QVariantList items;
        items << QVariant(inner);
        ctx.setProperty("items", items);
        ctx.setProperty("items[0]x", "Z");
        QCOMPARE(ctx.property("items[0].x").toString(), QString("y"));
    }

    // ---------- MapPropertyContext PropertyProxy 语法糖 ----------

    void testMapProxyRead()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        QVariant v = ctx["cpu"];
        QCOMPARE(v.toString(), QString("45%"));
    }

    void testMapProxyWrite()
    {
        BroadItem::MapPropertyContext ctx;
        ctx["cpu"] = "45%";
        QCOMPARE(ctx.property("cpu").toString(), QString("45%"));
    }

    void testMapProxyReadNested()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);

        QVariant v = ctx["device"]["cpu"];
        QCOMPARE(v.toString(), QString("45%"));
    }

    void testMapProxyWriteNested()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);

        ctx["device"]["cpu"] = "100%";
        QCOMPARE(ctx.property("device.cpu").toString(), QString("100%"));
    }

    void testMapProxyChain()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        QVariantMap item;
        item["name"] = "a";
        items << item;
        ctx.setProperty("items", items);

        QVariant v = ctx["items"][0]["name"];
        QCOMPARE(v.toString(), QString("a"));

        ctx["items"][0]["name"] = "b";
        QCOMPARE(ctx.property("items[0].name").toString(), QString("b"));
    }

    void testMapProxyFlatWriteNotify()
    {
        BroadItem::MapPropertyContext ctx;
        int count = 0;
        ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });

        ctx["cpu"] = "45%";
        QCOMPARE(count, 1);
    }

    void testMapProxyNestedWriteNotify()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);

        int count = 0;
        QString lastName;
        ctx.setOnChanged([&](const QString& n, const QVariant&) {
            ++count;
            lastName = n;
        });

        ctx["device"]["cpu"] = "100%";
        QCOMPARE(count, 1);
        QCOMPARE(lastName, QString("device"));
    }

    void testMapProxyReadSyntaxError()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("items", QVariantList{"a"});
        // 语法错误路径 → property 返回 invalid，proxy 应返回无效 QVariant
        QVariant v = ctx["items"][0]["unknown"];
        QVERIFY(!v.isValid());
    }
    // NOLINTEND(readability-convert-member-functions-to-static)
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void testQPropSetGet()
    {
        TestQProps ctx;
        ctx.setProperty("name", "test");
        QCOMPARE(ctx.property("name").toString(), QString("test"));
        QCOMPARE(ctx.name(), QString("test"));
    }

    void testQPropHasProperty()
    {
        TestQProps ctx;
        QVERIFY(ctx.hasProperty("name"));
        QVERIFY(ctx.hasProperty("tags"));
        QVERIFY(!ctx.hasProperty("nonexistent"));
    }

    void testQPropNoNotifyOnSame()
    {
        TestQProps ctx;
        int count = 0;
        ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });

        ctx.setProperty("name", "first");
        QCOMPARE(count, 1);

        ctx.setProperty("name", "first");
        QCOMPARE(count, 1);
    }

    void testQPropNullDoesNotRemove()
    {
        TestQProps ctx;
        ctx.setProperty("name", "hello");
        ctx.setProperty("name", QVariant());

        // Q_PROPERTY 始终存在
        QVERIFY(ctx.hasProperty("name"));
    }

    void testQPropMultiProperty()
    {
        TestQProps ctx;
        ctx.setProperty("name", "hello");
        ctx.setProperty("tags", QStringList{"a", "b"});

        QCOMPARE(ctx.property("name").toString(), QString("hello"));
        QStringList tags = ctx.property("tags").toStringList();
        QCOMPARE(tags.size(), 2);
        QCOMPARE(tags.at(0), QString("a"));
        QCOMPARE(tags.at(1), QString("b"));
    }

    void testQPropStringList()
    {
        TestQProps ctx;
        QStringList input;
        input << "tag1" << "tag2" << "tag3";
        ctx.setProperty("tags", QVariant(input));

        QStringList result = ctx.property("tags").toStringList();
        QCOMPARE(result.size(), 3);
        QCOMPARE(result.at(0), QString("tag1"));
    }

    void testQPropNotify()
    {
        TestQProps ctx;
        QString lastName;
        QVariant lastValue;
        ctx.setOnChanged([&](const QString& n, const QVariant& v) {
            lastName = n;
            lastValue = v;
        });

        ctx.setProperty("name", "notify-test");
        QCOMPARE(lastName, QString("name"));
        QCOMPARE(lastValue.toString(), QString("notify-test"));
    }

    void testQPropPathDot()
    {
        TestQProps ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        dev["mem"] = "60%";
        ctx.setProperty("device", dev);

        QCOMPARE(ctx.property("device.cpu").toString(), QString("45%"));
        QVERIFY(ctx.hasProperty("device.cpu"));
    }

    void testQPropPathBracket()
    {
        TestQProps ctx;
        QVariantList items;
        items.append("first");
        items.append("second");
        ctx.setProperty("items", items);

        QCOMPARE(ctx.property("items[1]").toString(), QString("second"));
    }

    void testQPropPathNested()
    {
        TestQProps ctx;
        QVariantMap proc;
        proc["name"] = "nginx";
        QVariantList procs;
        procs.append(proc);
        ctx.setProperty("items", procs);

        QCOMPARE(ctx.property("items[0].name").toString(), QString("nginx"));
    }

    void testQPropPathError()
    {
        TestQProps ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);

        QVERIFY(!ctx.property("device.ram").isValid());
        QVERIFY(!ctx.property("device.cpu.extra").isValid());
    }

    void testQPropDynamicProperty()
    {
        // 动态属性（未声明 Q_PROPERTY）→ event() 拦截 QDynamicPropertyChangeEvent
        TestQProps ctx;
        QString lastName;
        QVariant lastValue;
        ctx.setOnChanged([&](const QString& n, const QVariant& v) {
            lastName = n;
            lastValue = v;
        });

        ctx.setProperty("dynProp", "dynamic-value");
        QCOMPARE(lastName, QString("dynProp"));
        QCOMPARE(lastValue.toString(), QString("dynamic-value"));
    }

    void testQPropDirectSetterNotify()
    {
        // 直接调 Q_PROPERTY setter → emit NOTIFY → onNotify() → 回调
        // 通过 QTest::qWait 让 timer 触发自动连接
        TestQProps ctx;
        QTest::qWait(1);

        QString lastName;
        QVariant lastValue;
        ctx.setOnChanged([&](const QString& n, const QVariant& v) {
            lastName = n;
            lastValue = v;
        });

        ctx.setName("direct-value");
        QCOMPARE(lastName, QString("name"));
        QCOMPARE(lastValue.toString(), QString("direct-value"));
    }

    void testQPropHasPath()
    {
        TestQProps ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);

        QVERIFY(ctx.hasProperty("device.cpu"));
        QVERIFY(!ctx.hasProperty("device.ram"));
    }
    // NOLINTEND(readability-convert-member-functions-to-static)

    // ==================== QPropertyContext 嵌套写入 ====================
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    void testQPropSetNestedDot()
    {
        TestQProps ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        dev["mem"] = "60%";
        ctx.setProperty("device", dev);

        ctx.setProperty("device.cpu", "100%");
        QCOMPARE(ctx.property("device.cpu").toString(), QString("100%"));
        QCOMPARE(ctx.device()["cpu"].toString(), QString("100%"));
    }

    void testQPropSetNestedBracket()
    {
        TestQProps ctx;
        QVariantList items;
        items << "a" << "b" << "c";
        ctx.setProperty("items", items);

        ctx.setProperty("items[1]", "X");
        QCOMPARE(ctx.property("items[1]").toString(), QString("X"));
    }

    void testQPropSetNestedBracketDot()
    {
        TestQProps ctx;
        QVariantMap proc;
        proc["name"] = "nginx";
        QVariantList items;
        items << proc;
        ctx.setProperty("items", items);

        ctx.setProperty("items[0].name", "httpd");
        QCOMPARE(ctx.property("items[0].name").toString(), QString("httpd"));
    }

    void testQPropSetNestedMissingTopLevel()
    {
        TestQProps ctx;
        ctx.setProperty("nonexistent.cpu", "val");
        QVERIFY(true); // should not crash
    }

    void testQPropSetNestedSyntax()
    {
        TestQProps ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);
        // unmatched bracket → error, no crash
        ctx.setProperty("device[0", "X");
        QCOMPARE(ctx.property("device.cpu").toString(), QString("45%"));
    }
    // NOLINTEND(readability-convert-member-functions-to-static)

    void testItemSetGet()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);

        ctx.setProperty("status", "running");
        QCOMPARE(ctx.property("status").toString(), QString("running"));
        QCOMPARE(item.status(), QString("running"));
    }

    void testItemHasProperty()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);

        QVERIFY(ctx.hasProperty("status"));
        QVERIFY(ctx.hasProperty("level"));
        QVERIFY(!ctx.hasProperty("nonexistent"));
    }

    void testItemNoNotifyOnSame()
    {
        // NOTIFY 信号仅在 setter 确认值变更时才 emit，同值不通知
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        int count = 0;
        ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });

        ctx.setProperty("status", "running");
        QCOMPARE(count, 1);

        ctx.setProperty("status", "running");
        QCOMPARE(count, 1);
    }

    void testItemNullDoesNotRemove()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        ctx.setProperty("status", "hello");
        QVERIFY(ctx.hasProperty("status"));

        ctx.setProperty("status", QVariant());
        // Q_PROPERTY 始终存在
        QVERIFY(ctx.hasProperty("status"));
    }

    void testItemNotify()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QString lastName;
        QVariant lastValue;
        ctx.setOnChanged([&](const QString& n, const QVariant& v) {
            lastName = n;
            lastValue = v;
        });

        ctx.setProperty("status", "notify-me");
        QCOMPARE(lastName, QString("status"));
        QCOMPARE(lastValue.toString(), QString("notify-me"));
    }

    void testItemMultiProperty()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);

        ctx.setProperty("status", "running");
        ctx.setProperty("level", "warn");

        QCOMPARE(ctx.property("status").toString(), QString("running"));
        QCOMPARE(ctx.property("level").toString(), QString("warn"));
        QCOMPARE(item.status(), QString("running"));
        QCOMPARE(item.level(), QString("warn"));
    }

    void testItemPathDot()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QVariantMap cfg;
        cfg["host"] = "localhost";
        cfg["port"] = 8080;
        ctx.setProperty("config", cfg);

        QCOMPARE(ctx.property("config.host").toString(), QString("localhost"));
        QCOMPARE(ctx.property("config.port").toInt(), 8080);
        QVERIFY(ctx.hasProperty("config.host"));
    }

    void testItemPathBracket()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QVariantList nodes;
        nodes.append("node-a");
        nodes.append("node-b");
        ctx.setProperty("nodes", nodes);

        QCOMPARE(ctx.property("nodes[0]").toString(), QString("node-a"));
    }

    void testItemPathNested()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QVariantMap inner;
        inner["name"] = "worker";
        QVariantList nodes;
        nodes.append(inner);
        ctx.setProperty("nodes", nodes);

        QCOMPARE(ctx.property("nodes[0].name").toString(), QString("worker"));
    }

    void testItemPathError()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QVariantMap cfg;
        cfg["host"] = "localhost";
        ctx.setProperty("config", cfg);

        QVERIFY(!ctx.property("config.port").isValid());
    }

    void testItemDynamicProperty()
    {
        // 动态属性（item 上未声明 Q_PROPERTY）→ eventFilter 拦截
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QString lastName;
        QVariant lastValue;
        ctx.setOnChanged([&](const QString& n, const QVariant& v) {
            lastName = n;
            lastValue = v;
        });

        ctx.setProperty("dynProp", "from-item");
        QCOMPARE(lastName, QString("dynProp"));
        QCOMPARE(lastValue.toString(), QString("from-item"));
    }

    void testItemDirectSetterNotify()
    {
        // 直接调 item 的 Q_PROPERTY setter → emit NOTIFY → SignalBridge → 回调
        // 通过 QTest::qWait 让 timer 触发自动连接
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QTest::qWait(1);

        QString lastName;
        QVariant lastValue;
        ctx.setOnChanged([&](const QString& n, const QVariant& v) {
            lastName = n;
            lastValue = v;
        });

        item.setStatus("direct-from-item");
        QCOMPARE(lastName, QString("status"));
        QCOMPARE(lastValue.toString(), QString("direct-from-item"));
    }

    void testItemHasPath()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QVariantMap cfg;
        cfg["host"] = "localhost";
        ctx.setProperty("config", cfg);

        QVERIFY(ctx.hasProperty("config.host"));
        QVERIFY(!ctx.hasProperty("config.port"));
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void testItemNullItem()
    {
        BroadItem::QPropertyContext ctx(nullptr);

        QVERIFY(!ctx.property("anything").isValid());
        QVERIFY(!ctx.hasProperty("anything"));
        // setProperty on null item: should not crash
        ctx.setProperty("anything", "val");
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void testItemNullItemNotify()
    {
        // null target → 退回自宿主模式，动态属性仍会触发 event() 通知
        BroadItem::QPropertyContext ctx(nullptr);
        int count = 0;
        ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });

        ctx.setProperty("anything", "val");
        QCOMPARE(count, 1);
    }

    void testSignalBridgeDtorCleanup()
    {
        // SignalBridge 析构时应 removeEventFilter + disconnect，
        // 上下文销毁后 item 的直接 setter 不再触发通知
        TestPropItem item(m_tempXmlPath);
        int count = 0;

        {
            BroadItem::QPropertyContext ctx(&item);
            ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });
            ctx.setProperty("status", "first");
            QCOMPARE(count, 1);
        } // ctx / SignalBridge 析构

        item.setStatus("second");
        QCOMPARE(count, 1); // 析构后不再通知
    }
    // NOLINTEND(readability-convert-member-functions-to-static)

    // ==================== BroadItem operator[] ====================
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    void testItemProxyRead()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        ctx.setProperty("status", "running");

        QVariant v = ctx["status"];
        QCOMPARE(v.toString(), QString("running"));
    }

    void testItemProxyWrite()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        ctx["status"] = "from-proxy";

        QCOMPARE(ctx.property("status").toString(), QString("from-proxy"));
        QCOMPARE(item.status(), QString("from-proxy"));
    }

    void testItemProxyReadNested()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QVariantMap cfg;
        cfg["host"] = "localhost";
        ctx.setProperty("config", cfg);

        QVariant v = ctx["config"]["host"];
        QCOMPARE(v.toString(), QString("localhost"));
    }

    void testItemProxyWriteNested()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QVariantMap cfg;
        cfg["host"] = "localhost";
        ctx.setProperty("config", cfg);

        ctx["config"]["host"] = "example.com";
        QCOMPARE(ctx.property("config.host").toString(), QString("example.com"));
        QCOMPARE(item.config()["host"].toString(), QString("example.com"));
    }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestPropertyContext)
#include "test_property_context.moc"
