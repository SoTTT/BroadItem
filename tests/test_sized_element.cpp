#include <QtTest/QtTest>
#include "broaditem/XmlLayoutParser.h"
#include "broaditem/LayoutContext.h"
#include "broaditem/MapPropertyContext.h"
#include "broaditem/LayoutEngine.h"
#include "broaditem/Element.h"
#include "broaditem/elements/TextElement.h"
#include <QDebug>

class TestSizedElement : public QObject {
    Q_OBJECT

private slots:
    void testParseWidthHeight();
    void testMeasureWithSize();
    void testMeasureWithoutSize();
    void testClonePreservesSize();
    void testLayoutStretch();
};

void TestSizedElement::testParseWidthHeight()
{
    QString xml = R"(
        <root>
            <text width="100" height="50">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
}

void TestSizedElement::testMeasureWithSize()
{
    QString xml = R"(
        <root>
            <text width="100" height="50">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    auto ctx = std::make_shared<BroadItem::MapPropertyContext>();
    BroadItem::LayoutContext layoutCtx;
    layoutCtx.ctx = ctx.get();
    BroadItem::LayoutConstraints constraints;

    auto result = BroadItem::LayoutEngine::measure(root, layoutCtx, constraints);
    QCOMPARE(result.width, 100.0);
    QCOMPARE(result.height, 50.0);
}

void TestSizedElement::testMeasureWithoutSize()
{
    QString xml = R"(
        <root>
            <text>Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    auto ctx = std::make_shared<BroadItem::MapPropertyContext>();
    BroadItem::LayoutContext layoutCtx;
    layoutCtx.ctx = ctx.get();
    BroadItem::LayoutConstraints constraints;

    auto result = BroadItem::LayoutEngine::measure(root, layoutCtx, constraints);
    // Without width/height, size should be based on content
    QVERIFY(result.width > 0);
    QVERIFY(result.height > 0);
}

void TestSizedElement::testClonePreservesSize()
{
    auto text = std::make_shared<BroadItem::TextElement>();
    // Simulate parsing width/height
    QDomDocument doc;
    auto elem = doc.createElement("text");
    elem.setAttribute("width", "100");
    elem.setAttribute("height", "50");
    text->parse(elem);

    auto cloned = text->clone();
    auto clonedText = std::dynamic_pointer_cast<BroadItem::TextElement>(cloned);
    QVERIFY(clonedText != nullptr);
    QCOMPARE(clonedText->width(), 100.0);
    QCOMPARE(clonedText->height(), 50.0);
    QVERIFY(clonedText->hasWidth());
    QVERIFY(clonedText->hasHeight());
}

void TestSizedElement::testLayoutStretch()
{
    // Test that width/height are request sizes, layout can stretch
    QString xml = R"(
        <root>
            <row cross-align="stretch">
                <text width="100" height="50">Hello</text>
            </row>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    auto ctx = std::make_shared<BroadItem::MapPropertyContext>();
    BroadItem::LayoutContext layoutCtx;
    layoutCtx.ctx = ctx.get();
    BroadItem::LayoutConstraints constraints;
    constraints.availableWidth = 200;
    constraints.availableHeight = 100;

    auto result = BroadItem::LayoutEngine::measure(root, layoutCtx, constraints);
    BroadItem::Rect rect;
    rect.pos = {0, 0};
    rect.size = result;
    BroadItem::LayoutEngine::layout(root, layoutCtx, rect);
}

QTEST_MAIN(TestSizedElement)
#include "test_sized_element.moc"
