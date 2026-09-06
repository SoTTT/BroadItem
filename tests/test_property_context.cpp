#include <QtTest/QtTest>
#include <broaditem/context/PropertyContext.h>
#include <broaditem/context/MapPropertyContext.h>

// ============================================================
// 测试类
// ============================================================
class TestPropertyContext : public QObject {
    Q_OBJECT
private slots:
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

    /// @brief 病态路径统一判非法——语法权威在 Expression，
    /// 运行期不再各自解释（"device." 曾为运行期静默接受的分歧点）。
    void testMapPathInvalidSyntax()
    {
        BroadItem::MapPropertyContext ctx;
        QVariantMap device;
        device["cpu"] = "45%";
        ctx.setProperty("device", device);

        // 结尾点号与 ']' 后裸键均为语法错误，统一返回无效值（伴随 BI-R-005 诊断）。
        QVERIFY(!ctx.property("device.").isValid());
        QVERIFY(!ctx.property("device[0]cpu").isValid());
    }

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

    // ---------- 带路径的 hasProperty ----------

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
};

QTEST_MAIN(TestPropertyContext)
#include "test_property_context.moc"
