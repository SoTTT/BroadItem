#include <QtTest/QtTest>
#include <broaditem/core/Node.h>
#include <broaditem/core/LayoutEngine.h>
#include <broaditem/element/layout/RowLayout.h>
#include <broaditem/element/layout/CellElement.h>
#include <broaditem/element/control/ForElement.h>
#include <broaditem/element/control/IfElement.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>

namespace {

/// @brief 最小叶子元素：仅实现纯虚 materialize()，用作 for 模板或普通子元素。
///
/// 物化时产生一个裸节点，其 element 指针指回该模板自身。
struct SimpleLeaf : BroadItem::Element {
    std::unique_ptr<BroadItem::Node> materialize(const BroadItem::LayoutContext&) const override
    {
        auto node = std::make_unique<BroadItem::Node>();
        node->element = this;
        return node;
    }
};

/// @brief 构造 ForElement：b:of=of、b:as=as，模板为 templ。
/// @param of 迭代列表的绑定属性名。
/// @param as 迭代变量名。
/// @param templ 每次迭代要物化的模板元素。
std::shared_ptr<BroadItem::ForElement> makeFor(const QString& of, const QString& as,
                                               BroadItem::ElementPtr templ)
{
    auto forEl = std::make_shared<BroadItem::ForElement>();
    forEl->setBindProperty(of);
    forEl->setAsVariable(as);
    forEl->setTemplate(std::move(templ));
    return forEl;
}

/// @brief 构造 IfElement（存在性形态）：b:prop=prop，条件子元素为 child。
/// @param prop 条件检查的属性名。
/// @param child 条件成立时要物化的子元素。
std::shared_ptr<BroadItem::IfElement> makeIf(const QString& prop,
                                             BroadItem::ElementPtr child)
{
    auto ifEl = std::make_shared<BroadItem::IfElement>();
    ifEl->setBindProperty(prop);
    ifEl->setChild(std::move(child));
    return ifEl;
}

/// @brief 构造 IfElement（值比较形态）：b:prop=prop，equals 字面量比较。
/// @param prop 条件检查的属性名。
/// @param equals 字面量比较值。
/// @param child 条件成立时要物化的子元素。
std::shared_ptr<BroadItem::IfElement> makeIfEquals(const QString& prop, const QString& equals,
                                                   BroadItem::ElementPtr child)
{
    auto ifEl = std::make_shared<BroadItem::IfElement>();
    ifEl->setBindProperty(prop);
    ifEl->setEquals(equals);
    ifEl->setChild(std::move(child));
    return ifEl;
}

} // anonymous namespace

/// @brief 验证物化（materialize）展开语义：控制元素（for、if）在物化时
/// 结构性消失，node->children 即旧架构 flattenChildren() 的"展平结果"。
class TestFlattenChildren : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    /// @brief for 产生 N 个节点（旧 testFlattenForElement 的物化版本）。
    void testForProducesNNodes()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"a", "b", "c"});
        BroadItem::LayoutContext ctx{&mapCtx};

        auto leaf = std::make_shared<SimpleLeaf>();
        BroadItem::RowLayout container;
        container.addChild(makeFor("items", "v", leaf));

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        // 容器节点自身指回容器模板
        QVERIFY(node->element == &container);
        // for 控制元素结构性消失，3 次迭代产生 3 个子节点
        QCOMPARE(node->children.size(), 3);
        // 每个迭代节点的 element 均指向同一叶子模板
        for (const auto& child : node->children)
            QVERIFY(child->element == leaf.get());
    }

    /// @brief if 条件成立产生 1 个节点（旧 testFlattenIfHasElementPresent）。
    void testIfPresentProducesOneNode()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("show", true);
        BroadItem::LayoutContext ctx{&mapCtx};

        auto leaf = std::make_shared<SimpleLeaf>();
        BroadItem::RowLayout container;
        container.addChild(makeIf("show", leaf));

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        QCOMPARE(node->children.size(), 1);
        QVERIFY(node->children[0]->element == leaf.get());
    }

    /// @brief if 条件不成立产生 0 个节点（旧 testFlattenIfHasElementAbsent）。
    void testIfAbsentProducesZeroNodes()
    {
        BroadItem::MapPropertyContext mapCtx;
        // 不设置 "show" 属性
        BroadItem::LayoutContext ctx{&mapCtx};

        BroadItem::RowLayout container;
        container.addChild(makeIf("show", std::make_shared<SimpleLeaf>()));

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        QCOMPARE(node->children.size(), 0);
    }

    /// @brief if 值比较（equals）：比较成立展开 1 个节点，不成立展开 0 个节点。
    void testIfEqualsFlatten()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("status", QStringLiteral("alarm"));
        BroadItem::LayoutContext ctx{&mapCtx};

        BroadItem::RowLayout container;
        container.addChild(makeIfEquals("status", "alarm", std::make_shared<SimpleLeaf>()));  // 成立 → 1
        container.addChild(makeIfEquals("status", "ok", std::make_shared<SimpleLeaf>()));     // 不成立 → 0

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        QCOMPARE(node->children.size(), 1);
    }

    /// @brief 空容器物化后无子节点（旧 testFlattenEmptyChildren）。
    void testEmptyContainerProducesNoChildren()
    {
        BroadItem::MapPropertyContext mapCtx;
        BroadItem::LayoutContext ctx{&mapCtx};

        BroadItem::RowLayout container;
        // 不添加任何子元素

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        QVERIFY(node->children.empty());
    }

    /// @brief for 绑定空列表产生 0 个节点（旧 testFlattenForElementEmptyList）。
    void testForEmptyListProducesZeroNodes()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{});
        BroadItem::LayoutContext ctx{&mapCtx};

        BroadItem::RowLayout container;
        container.addChild(makeFor("items", "v", std::make_shared<SimpleLeaf>()));

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        QCOMPARE(node->children.size(), 0);
    }

    /// @brief 普通子元素与 for 混排：普通元素透传 + for 展开（旧 testFlattenMixedControlAndNormal）。
    void testMixedControlAndNormal()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"x", "y"});
        BroadItem::LayoutContext ctx{&mapCtx};

        auto normalLeaf = std::make_shared<SimpleLeaf>();
        auto forLeaf = std::make_shared<SimpleLeaf>();

        BroadItem::RowLayout container;
        container.addChild(normalLeaf);                        // 普通子元素
        container.addChild(makeFor("items", "v", forLeaf));    // 控制子元素

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        // 1 个普通 + 2 个 for 展开 = 3，且保持声明顺序
        QCOMPARE(node->children.size(), 3);
        QVERIFY(node->children[0]->element == normalLeaf.get());
        QVERIFY(node->children[1]->element == forLeaf.get());
        QVERIFY(node->children[2]->element == forLeaf.get());
    }

    /// @brief for 与 if 混排：按条件分别展开为 1/2/0 个节点。
    void testForAndIfMixed()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("show", true);
        mapCtx.setProperty("items", QStringList{"x", "y"});
        BroadItem::LayoutContext ctx{&mapCtx};

        auto ifLeaf = std::make_shared<SimpleLeaf>();
        auto forLeaf = std::make_shared<SimpleLeaf>();

        BroadItem::RowLayout container;
        container.addChild(makeIf("show", ifLeaf));                  // 条件成立 → 1
        container.addChild(makeFor("items", "v", forLeaf));          // 2 项 → 2
        container.addChild(makeIf("missing", std::make_shared<SimpleLeaf>()));  // 不成立 → 0

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        // 1 + 2 + 0 = 3
        QCOMPARE(node->children.size(), 3);
        QVERIFY(node->children[0]->element == ifLeaf.get());
        QVERIFY(node->children[1]->element == forLeaf.get());
        QVERIFY(node->children[2]->element == forLeaf.get());
    }

    /// @brief cell 容器与控制元素混排：cell 透传并物化自身内容（旧 testFlattenGridWithControlInCells）。
    void testCellPassesThroughWithControl()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"a"});
        BroadItem::LayoutContext ctx{&mapCtx};

        // CellElement 含普通 SimpleLeaf —— 作为普通子元素透传
        auto cellContent = std::make_shared<SimpleLeaf>();
        auto cell = std::make_shared<BroadItem::CellElement>();
        cell->setContent(cellContent);

        auto forLeaf = std::make_shared<SimpleLeaf>();

        BroadItem::RowLayout container;
        container.addChild(cell);
        container.addChild(makeFor("items", "v", forLeaf));

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        // cell 透传 + for 展开 1 项 = 2
        QCOMPARE(node->children.size(), 2);
        QVERIFY(node->children[0]->element == cell.get());
        // cell 节点自身物化了 1 个内容子节点
        QCOMPARE(node->children[0]->children.size(), 1);
        QVERIFY(node->children[0]->children[0]->element == cellContent.get());
        QVERIFY(node->children[1]->element == forLeaf.get());
    }

    /// @brief 嵌套包装（元素带 b:of 被包装）：for 模板为容器时，每个迭代节点
    /// 都是该容器模板的实例，且容器自身的子元素也被递归物化。
    void testForWrappingContainerTemplate()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"a", "b"});
        BroadItem::LayoutContext ctx{&mapCtx};

        // 等价于 XML 中 <row b:of="items" b:as="v">：RowLayout 被 ForElement 包装
        auto innerLeaf = std::make_shared<SimpleLeaf>();
        auto row = std::make_shared<BroadItem::RowLayout>();
        row->addChild(innerLeaf);
        row->addChild(std::make_shared<SimpleLeaf>());

        BroadItem::RowLayout container;
        container.addChild(makeFor("items", "v", row));

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        // 2 次迭代 → 2 个 RowLayout 实例节点
        QCOMPARE(node->children.size(), 2);
        for (const auto& child : node->children) {
            QVERIFY(child->element == row.get());
            // 每个实例节点递归物化了行内的 2 个子元素
            QCOMPARE(child->children.size(), 2);
            QVERIFY(child->children[0]->element == innerLeaf.get());
        }
    }

    /// @brief 嵌套包装（元素带 b:prop 被包装）：if 位于 for 模板内时，
    /// 条件按 per-item 上下文逐项求值，仅条件成立的迭代产生节点。
    void testForWrappingIfPerItem()
    {
        BroadItem::MapPropertyContext mapCtx;
        QVariantList items;
        items.append(QVariantMap{{"name", "a"}, {"show", true}});
        items.append(QVariantMap{{"name", "b"}});  // 无 show 键 → 条件不成立
        items.append(QVariantMap{{"name", "c"}, {"show", true}});
        mapCtx.setProperty("items", items);
        BroadItem::LayoutContext ctx{&mapCtx};

        // 等价于 XML 中 <for> 内 <text b:prop="show">：叶子被 IfElement 包装
        auto leaf = std::make_shared<SimpleLeaf>();
        BroadItem::RowLayout container;
        container.addChild(makeFor("items", "item", makeIf("show", leaf)));

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        // 3 项中仅 2 项条件成立 → 2 个节点
        QCOMPARE(node->children.size(), 2);
        QVERIFY(node->children[0]->element == leaf.get());
        QVERIFY(node->children[1]->element == leaf.get());
    }

    /// @brief 无效项跳过：QVariantList 中的 null 项被跳过并告警，仅有效项物化。
    void testForSkipsNullItems()
    {
        BroadItem::MapPropertyContext mapCtx;
        QVariantList list;
        list.append(QVariantMap{{"name", "Alice"}});
        list.append(QVariant());   // null 项
        list.append(QVariantMap{{"name", "Carol"}});
        mapCtx.setProperty("items", list);
        BroadItem::LayoutContext ctx{&mapCtx};

        auto leaf = std::make_shared<SimpleLeaf>();
        BroadItem::RowLayout container;
        container.addChild(makeFor("items", "item", leaf));

        // 跳过 null 项时输出 qWarning
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*skipping null item.*"));

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        // null 项被跳过 → 仅 2 个有效节点
        QCOMPARE(node->children.size(), 2);
        QVERIFY(node->children[0]->element == leaf.get());
        QVERIFY(node->children[1]->element == leaf.get());
    }

    /// @brief 嵌套 for：内层 for 经 per-item 上下文拿到外层 as 变量（新架构已修复，
    /// 旧架构下内层必然展开为空）。外层每组产生 1 个行节点，内层在行内展开。
    void testNestedForExpandsInner()
    {
        BroadItem::MapPropertyContext mapCtx;
        QVariantList groups;
        groups.append(QVariantMap{{"members", QStringList{"a", "b"}}});
        groups.append(QVariantMap{{"members", QStringList{"c", "d", "e"}}});
        mapCtx.setProperty("groups", groups);
        BroadItem::LayoutContext ctx{&mapCtx};

        // 外层 for b:of="groups" b:as="g"，模板为 RowLayout；
        // 行内嵌套内层 for b:of="g.members" b:as="m"
        auto leaf = std::make_shared<SimpleLeaf>();
        auto row = std::make_shared<BroadItem::RowLayout>();
        row->addChild(makeFor("g.members", "m", leaf));

        BroadItem::RowLayout container;
        container.addChild(makeFor("groups", "g", row));

        auto node = container.materialize(ctx);
        QVERIFY(node != nullptr);
        // 外层 2 组 → 2 个行节点
        QCOMPARE(node->children.size(), 2);
        QVERIFY(node->children[0]->element == row.get());
        QVERIFY(node->children[1]->element == row.get());
        // 内层按各组 members 展开：第 1 组 2 个节点，第 2 组 3 个节点
        QCOMPARE(node->children[0]->children.size(), 2);
        QCOMPARE(node->children[1]->children.size(), 3);
        for (const auto& grandChild : node->children[0]->children)
            QVERIFY(grandChild->element == leaf.get());
        for (const auto& grandChild : node->children[1]->children)
            QVERIFY(grandChild->element == leaf.get());
    }

    /// @brief 经 LayoutEngine 公开入口物化：容器模板产生含全部展开子节点的根节点。
    void testLayoutEngineMaterializeEntry()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"a", "b", "c"});
        BroadItem::LayoutContext ctx{&mapCtx};

        auto leaf = std::make_shared<SimpleLeaf>();
        auto container = std::make_shared<BroadItem::RowLayout>();
        container->addChild(makeFor("items", "v", leaf));

        auto node = BroadItem::LayoutEngine::materialize(container, ctx);
        QVERIFY(node != nullptr);
        QVERIFY(node->element == container.get());
        QCOMPARE(node->children.size(), 3);
        for (const auto& child : node->children)
            QVERIFY(child->element == leaf.get());
    }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestFlattenChildren)
#include "test_flatten_children.moc"
