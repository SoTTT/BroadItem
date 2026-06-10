#include <QtTest/QtTest>
#include "broaditem/elements/MultiChildContainer.h"
#include "broaditem/elements/ForElement.h"
#include "broaditem/elements/IfHasElement.h"
#include "broaditem/elements/CellElement.h"
#include "broaditem/LayoutContext.h"
#include "broaditem/MapPropertyContext.h"

namespace {

/// @brief Minimal leaf element used as ForElement template / normal child in tests.
struct SimpleLeaf : BroadItem::Element {
    BroadItem::MeasureResult measure(const BroadItem::LayoutContext&,
                                     const BroadItem::LayoutConstraints&) override
    {
        return {QSizeF(10, 10)};
    }
    void layout(const BroadItem::LayoutContext&, const QRectF& rect) override { m_rect = rect; }
    void render(QPainter*, const BroadItem::LayoutContext&) const override {}

    BroadItem::ElementPtr clone() const override
    {
        return std::make_shared<SimpleLeaf>();
    }
};

/// @brief Test helper that exposes MultiChildContainer's protected members.
class TestContainer : public BroadItem::MultiChildContainer {
public:
    using MultiChildContainer::flattenChildren;
    using MultiChildContainer::m_children;
};

} // anonymous namespace

class TestFlattenChildren : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void testFlattenForElement()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"a", "b", "c"});
        BroadItem::LayoutContext ctx{&mapCtx};

        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");
        forEl->setAsVariable("v");
        forEl->setTemplate(std::make_shared<SimpleLeaf>());

        TestContainer container;
        container.addChild(forEl);

        auto flat = container.flattenChildren(ctx);
        QCOMPARE(flat.size(), 3);
    }

    void testFlattenIfHasElementPresent()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("show", true);
        BroadItem::LayoutContext ctx{&mapCtx};

        auto ifEl = std::make_shared<BroadItem::IfHasElement>();
        ifEl->setBindProperty("show");
        ifEl->setChild(std::make_shared<SimpleLeaf>());

        TestContainer container;
        container.addChild(ifEl);

        auto flat = container.flattenChildren(ctx);
        QCOMPARE(flat.size(), 1);
    }

    void testFlattenIfHasElementAbsent()
    {
        BroadItem::MapPropertyContext mapCtx;
        // Do NOT set "show" property
        BroadItem::LayoutContext ctx{&mapCtx};

        auto ifEl = std::make_shared<BroadItem::IfHasElement>();
        ifEl->setBindProperty("show");
        ifEl->setChild(std::make_shared<SimpleLeaf>());

        TestContainer container;
        container.addChild(ifEl);

        auto flat = container.flattenChildren(ctx);
        QCOMPARE(flat.size(), 0);
    }

    void testFlattenEmptyChildren()
    {
        BroadItem::MapPropertyContext mapCtx;
        BroadItem::LayoutContext ctx{&mapCtx};

        TestContainer container;
        // No children added

        auto flat = container.flattenChildren(ctx);
        QVERIFY(flat.empty());
    }

    void testFlattenForElementEmptyList()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{});
        BroadItem::LayoutContext ctx{&mapCtx};

        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");
        forEl->setAsVariable("v");
        forEl->setTemplate(std::make_shared<SimpleLeaf>());

        TestContainer container;
        container.addChild(forEl);

        auto flat = container.flattenChildren(ctx);
        QCOMPARE(flat.size(), 0);
    }

    void testFlattenMixedControlAndNormal()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"x", "y"});
        BroadItem::LayoutContext ctx{&mapCtx};

        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");
        forEl->setAsVariable("v");
        forEl->setTemplate(std::make_shared<SimpleLeaf>());

        TestContainer container;
        container.addChild(std::make_shared<SimpleLeaf>());  // normal child
        container.addChild(forEl);                             // control child

        auto flat = container.flattenChildren(ctx);
        // 1 normal + 2 from ForElement = 3
        QCOMPARE(flat.size(), 3);
    }

    void testFlattenGridWithControlInCells()
    {
        BroadItem::MapPropertyContext mapCtx;
        mapCtx.setProperty("items", QStringList{"a"});
        BroadItem::LayoutContext ctx{&mapCtx};

        // CellElement containing a normal SimpleLeaf — should pass through as-is
        auto cell = std::make_shared<BroadItem::CellElement>();
        cell->setContent(std::make_shared<SimpleLeaf>());

        // ForElement that expands to 1 clone
        auto forEl = std::make_shared<BroadItem::ForElement>();
        forEl->setBindProperty("items");
        forEl->setAsVariable("v");
        forEl->setTemplate(std::make_shared<SimpleLeaf>());

        TestContainer container;
        container.addChild(cell);
        container.addChild(forEl);

        auto flat = container.flattenChildren(ctx);
        // CellElement passes through + 1 from ForElement = 2
        QCOMPARE(flat.size(), 2);
    }
    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestFlattenChildren)
#include "test_flatten_children.moc"
