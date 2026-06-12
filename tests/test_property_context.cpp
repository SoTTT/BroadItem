#include <QtTest/QtTest>
#include <broaditem/context/PropertyContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/context/QPropertyContext.h>
#include <broaditem/core/BroadItem.h>
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
    /// @brief 测试 MapPropertyContext 的基本设置与读取
    void testMapSetGet()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        QCOMPARE(ctx.property("cpu").toString(), QString("45%"));
    }

    /// @brief 测试 MapPropertyContext 的 hasProperty 判断
    void testMapHasProperty()
    {
        BroadItem::MapPropertyContext ctx;
        QVERIFY(!ctx.hasProperty("cpu"));
        ctx.setProperty("cpu", "45%");
        QVERIFY(ctx.hasProperty("cpu"));
    }

    /// @brief 测试设为 null 时移除属性
    void testMapNullRemoves()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        QVERIFY(ctx.hasProperty("cpu"));

        ctx.setProperty("cpu", QVariant());
        QVERIFY(!ctx.hasProperty("cpu"));
        QVERIFY(!ctx.property("cpu").isValid());
    }

    /// @brief 测试 null 值使属性失效
    void testMapNullRemovesInvalid()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        ctx.setProperty("cpu", QVariant());
        QVERIFY(!ctx.hasProperty("cpu"));
    }

    /// @brief 测试相同值不触发通知
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

    /// @brief 测试扁平键的路径读取
    void testMapPathFlat()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        QCOMPARE(ctx.property("cpu").toString(), QString("45%"));
        QVERIFY(ctx.hasProperty("cpu"));
    }

    /// @brief 测试含点的键名不会误匹配路径，路径遍历不会找到顶层不存在的段
    void testMapPathFlatWithDotInName()
    {
        // 含 . 的 key 触发路径遍历，不会匹配顶层 map 中的这个键（因为段 0 不存在）
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("a.b", "val");
        // "a.b" 会先找顶层 "a"，如果 "a" 不存在 → 报错
        QVERIFY(!ctx.property("a.b").isValid());
    }

    // ---------- 路径：点号 ----------

    /// @brief 测试点号路径读取嵌套 Map
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

    /// @brief 测试深层点号路径（三层以上嵌套）
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

    /// @brief 测试下标路径读取列表元素
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

    /// @brief 测试下标+点号的混合嵌套路径（列表中的对象）
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

    /// @brief 测试多维下标路径（二维列表）
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

    /// @brief 测试访问不存在的嵌套键返回无效
    void testMapPathKeyNotFound()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("device", QVariantMap{});
        QVERIFY(!ctx.property("device.unknown").isValid());
    }

    /// @brief 测试对非 Map 值使用点号路径返回无效
    void testMapPathNotObject()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("val", "hello");
        // "val.anything" → val 是 QString，不是 map
        QVERIFY(!ctx.property("val.anything").isValid());
    }

    /// @brief 测试对非数组值使用下标路径返回无效
    void testMapPathNotArray()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("val", "hello");
        // "val[0]" → val 是 QString，不是 list
        QVERIFY(!ctx.property("val[0]").isValid());
    }

    /// @brief 测试下标越界返回无效
    void testMapPathIndexOutOfBounds()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items.append("only");
        ctx.setProperty("items", items);

        QVERIFY(!ctx.property("items[3]").isValid());
    }

    // ---------- 语法错误 ----------

    /// @brief 测试空字符串路径返回无效
    void testMapPathSyntaxEmpty()
    {
        BroadItem::MapPropertyContext ctx;
        QVERIFY(!ctx.property("").isValid());
        QVERIFY(!ctx.hasProperty(""));
    }

    /// @brief 测试空键段路径（连续点号）返回无效
    void testMapPathSyntaxEmptyKey()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap map;
        map["a"] = "val";
        ctx.setProperty("root", map);
        QVERIFY(!ctx.property("root..a").isValid());
    }

    /// @brief 测试未闭合括号的语法错误返回无效
    void testMapPathSyntaxUnmatched()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items.append("x");
        ctx.setProperty("items", items);
        QVERIFY(!ctx.property("items[0").isValid());
    }

    /// @brief 测试非法下标（非数字）语法错误返回无效
    void testMapPathSyntaxBadIndex()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items.append("x");
        ctx.setProperty("items", items);
        QVERIFY(!ctx.property("items[abc]").isValid());
    }

    /// @brief 测试负下标被拒绝返回无效
    void testMapPathSyntaxNegative()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items.append("x");
        ctx.setProperty("items", items);
        QVERIFY(!ctx.property("items[-1]").isValid());
    }

    /// @brief 测试下标后缺少点号直接接键名的语法错误返回无效
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

    /// @brief 测试 hasProperty 对路径的正确判断
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

    /// @brief 测试顶层缺失时路径 hasProperty 返回 false
    void testMapHasPathTopLevelMissing()
    {
        BroadItem::MapPropertyContext ctx;
        QVERIFY(!ctx.hasProperty("device.cpu"));
    }

    // ---------- 回调通知 ----------

    /// @brief 测试设置属性触发回调，验证名称和值正确
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

    /// @brief 测试设 null 触发回调
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

    /// @brief 测试多次设置触发多次回调
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

    /// @brief 测试删除不存在的 key 不触发通知
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
    /// @brief 测试通过点号路径写入嵌套 Map
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

    /// @brief 测试通过下标路径写入列表元素
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

    /// @brief 测试通过下标+点号写入嵌套结构
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

    /// @brief 测试深层嵌套路径写入
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

    /// @brief 测试多维下标写入
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

    /// @brief 测试嵌套写入触发父属性通知
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

    /// @brief 测试写入不存在的顶层路径不崩溃也不写入
    void testMapSetNestedMissingTopLevel()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("nonexistent.cpu", "val");
        // 首段不存在 → 不写入，不抛出
        QVERIFY(true);
    }

    /// @brief 测试对字面量值使用点号写入不生效
    void testMapSetNestedNotMap()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("str", "hello");
        ctx.setProperty("str.key", "val");
        // "str" 是字面量，不是 map → 不写入
        QCOMPARE(ctx.property("str").toString(), QString("hello"));
    }

    /// @brief 测试对非列表使用下标写入不生效
    void testMapSetNestedNotList()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("str", "hello");
        ctx.setProperty("str[0]", "val");
        // "str" 不是 list → 不写入
        QCOMPARE(ctx.property("str").toString(), QString("hello"));
    }

    /// @brief 测试下标越界写入不生效
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

    /// @brief 测试连续点号语法错误时不写入
    void testMapSetNestedSyntaxDoubleDot()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap root;
        root["a"] = "val";
        ctx.setProperty("root", root);
        ctx.setProperty("root..a", "X");
        QCOMPARE(ctx.property("root.a").toString(), QString("val"));
    }

    /// @brief 测试末尾点号语法错误时不写入
    void testMapSetNestedSyntaxTrailingDot()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap root;
        root["a"] = "val";
        ctx.setProperty("root", root);
        ctx.setProperty("root.", "X");
        QCOMPARE(ctx.property("root.a").toString(), QString("val"));
    }

    /// @brief 测试未闭合括号语法错误时不写入
    void testMapSetNestedSyntaxUnmatched()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items << "x";
        ctx.setProperty("items", items);
        ctx.setProperty("items[0", "X");
        QCOMPARE(ctx.property("items[0]").toString(), QString("x"));
    }

    /// @brief 测试非法下标语法错误时不写入
    void testMapSetNestedSyntaxBadIndex()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items << "x";
        ctx.setProperty("items", items);
        ctx.setProperty("items[abc]", "X");
        QCOMPARE(ctx.property("items[0]").toString(), QString("x"));
    }

    /// @brief 测试负下标语法错误时不写入
    void testMapSetNestedSyntaxNegative()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantList items;
        items << "x";
        ctx.setProperty("items", items);
        ctx.setProperty("items[-1]", "X");
        QCOMPARE(ctx.property("items[0]").toString(), QString("x"));
    }

    /// @brief 测试下标后缺点的语法错误时不写入
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

    /// @brief 测试 operator[] 读取属性
    void testMapProxyRead()
    {
        BroadItem::MapPropertyContext ctx;
        ctx.setProperty("cpu", "45%");
        QVariant v = ctx["cpu"];
        QCOMPARE(v.toString(), QString("45%"));
    }

    /// @brief 测试 operator[] 写入属性
    void testMapProxyWrite()
    {
        BroadItem::MapPropertyContext ctx;
        ctx["cpu"] = "45%";
        QCOMPARE(ctx.property("cpu").toString(), QString("45%"));
    }

    /// @brief 测试 operator[] 链式读取嵌套属性
    void testMapProxyReadNested()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);

        QVariant v = ctx["device"]["cpu"];
        QCOMPARE(v.toString(), QString("45%"));
    }

    /// @brief 测试 operator[] 链式写入嵌套属性
    void testMapProxyWriteNested()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);

        ctx["device"]["cpu"] = "100%";
        QCOMPARE(ctx.property("device.cpu").toString(), QString("100%"));
    }

    /// @brief 测试 operator[] 多重链式读写（列表→对象→键）
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

    /// @brief 测试 operator[] 写入触发通知
    void testMapProxyFlatWriteNotify()
    {
        BroadItem::MapPropertyContext ctx;
        int count = 0;
        ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });

        ctx["cpu"] = "45%";
        QCOMPARE(count, 1);
    }

    /// @brief 测试 operator[] 嵌套写入触发父属性通知
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

    /// @brief 测试 operator[] 语法错误路径返回无效 QVariant
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
    /// @brief 测试 QPropertyContext 的基本设置与读取
    void testQPropSetGet()
    {
        TestQProps ctx;
        ctx.setProperty("name", "test");
        QCOMPARE(ctx.property("name").toString(), QString("test"));
        QCOMPARE(ctx.name(), QString("test"));
    }

    /// @brief 测试 Q_PROPERTY 的 hasProperty 能正确识别声明的属性
    void testQPropHasProperty()
    {
        TestQProps ctx;
        QVERIFY(ctx.hasProperty("name"));
        QVERIFY(ctx.hasProperty("tags"));
        QVERIFY(!ctx.hasProperty("nonexistent"));
    }

    /// @brief 测试相同值不触发通知
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

    /// @brief 测试 Q_PROPERTY 设 null 后 hasProperty 仍返回 true（属性始终存在）
    void testQPropNullDoesNotRemove()
    {
        TestQProps ctx;
        ctx.setProperty("name", "hello");
        ctx.setProperty("name", QVariant());

        // Q_PROPERTY 始终存在
        QVERIFY(ctx.hasProperty("name"));
    }

    /// @brief 测试多属性同时读写
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

    /// @brief 测试 QStringList 属性的读写
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

    /// @brief 测试设置属性触发回调，验证名称和值
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

    /// @brief 测试 Q_PROPERTY 返回值中通过点号路径访问嵌套 Map
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

    /// @brief 测试 Q_PROPERTY 返回值中通过下标路径访问列表
    void testQPropPathBracket()
    {
        TestQProps ctx;
        QVariantList items;
        items.append("first");
        items.append("second");
        ctx.setProperty("items", items);

        QCOMPARE(ctx.property("items[1]").toString(), QString("second"));
    }

    /// @brief 测试 Q_PROPERTY 返回值中混合嵌套路径（下标+点号）
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

    /// @brief 测试 Q_PROPERTY 路径不存在时返回无效
    void testQPropPathError()
    {
        TestQProps ctx;
        QVariantMap dev;
        dev["cpu"] = "45%";
        ctx.setProperty("device", dev);

        QVERIFY(!ctx.property("device.ram").isValid());
        QVERIFY(!ctx.property("device.cpu.extra").isValid());
    }

    /// @brief 测试动态属性通过 setProperty 触发回调
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

    /// @brief 测试直接调用 Q_PROPERTY setter 通过 NOTIFY 信号触发回调
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

    /// @brief 测试 Q_PROPERTY 路径的 hasProperty 判断
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

    /// @brief 测试通过点号路径写入嵌套 Map
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

    /// @brief 测试通过下标路径写入列表
    void testQPropSetNestedBracket()
    {
        TestQProps ctx;
        QVariantList items;
        items << "a" << "b" << "c";
        ctx.setProperty("items", items);

        ctx.setProperty("items[1]", "X");
        QCOMPARE(ctx.property("items[1]").toString(), QString("X"));
    }

    /// @brief 测试混合下标+点号写入
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

    /// @brief 测试写入不存在的顶层路径不崩溃
    void testQPropSetNestedMissingTopLevel()
    {
        TestQProps ctx;
        ctx.setProperty("nonexistent.cpu", "val");
        QVERIFY(true); // should not crash
    }

    /// @brief 测试语法错误路径写入不生效也不崩溃
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

    /// @brief 测试 BroadItem 属性上下文的基本设置与读取
    void testItemSetGet()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);

        ctx.setProperty("status", "running");
        QCOMPARE(ctx.property("status").toString(), QString("running"));
        QCOMPARE(item.status(), QString("running"));
    }

    /// @brief 测试 BroadItem 上 Q_PROPERTY 的 hasProperty 识别
    void testItemHasProperty()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);

        QVERIFY(ctx.hasProperty("status"));
        QVERIFY(ctx.hasProperty("level"));
        QVERIFY(!ctx.hasProperty("nonexistent"));
    }

    /// @brief 测试相同值不触发通知
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

    /// @brief 测试 Q_PROPERTY 设 null 后 hasProperty 仍返回 true
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

    /// @brief 测试设置属性触发回调
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

    /// @brief 测试多属性同时读写
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

    /// @brief 测试嵌套 Map 的点号路径读取
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

    /// @brief 测试列表下标路径读取
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

    /// @brief 测试混合嵌套路径（下标+点号）
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

    /// @brief 测试路径不存在返回无效
    void testItemPathError()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        QVariantMap cfg;
        cfg["host"] = "localhost";
        ctx.setProperty("config", cfg);

        QVERIFY(!ctx.property("config.port").isValid());
    }

    /// @brief 测试动态属性通过 eventFilter 触发回调
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

    /// @brief 测试直接调 item 的 Q_PROPERTY setter 通过 SignalBridge 触发回调
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

    /// @brief 测试 hasProperty 路径判断
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
    /// @brief 测试传入 nullptr 的 QPropertyContext 不崩溃
    void testItemNullItem()
    {
        BroadItem::QPropertyContext ctx(nullptr);

        QVERIFY(!ctx.property("anything").isValid());
        QVERIFY(!ctx.hasProperty("anything"));
        // setProperty on null item: should not crash
        ctx.setProperty("anything", "val");
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    /// @brief 测试 null 目标退回自宿主模式，动态属性仍可触发通知
    void testItemNullItemNotify()
    {
        // null target → 退回自宿主模式，动态属性仍会触发 event() 通知
        BroadItem::QPropertyContext ctx(nullptr);
        int count = 0;
        ctx.setOnChanged([&](const QString&, const QVariant&) { ++count; });

        ctx.setProperty("anything", "val");
        QCOMPARE(count, 1);
    }

    /// @brief 测试 SignalBridge 析构后 removeEventFilter + disconnect，不再触发通知
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

    /// @brief 测试 operator[] 读取属性
    void testItemProxyRead()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        ctx.setProperty("status", "running");

        QVariant v = ctx["status"];
        QCOMPARE(v.toString(), QString("running"));
    }

    /// @brief 测试 operator[] 写入属性
    void testItemProxyWrite()
    {
        TestPropItem item(m_tempXmlPath);
        BroadItem::QPropertyContext ctx(&item);
        ctx["status"] = "from-proxy";

        QCOMPARE(ctx.property("status").toString(), QString("from-proxy"));
        QCOMPARE(item.status(), QString("from-proxy"));
    }

    /// @brief 测试 operator[] 链式读取嵌套属性
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

    /// @brief 测试 operator[] 链式写入嵌套属性
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
