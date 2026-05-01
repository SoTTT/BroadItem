#include <QtTest/QtTest>
#include "broaditem/XmlLayoutParser.h"
#include "broaditem/LayoutContext.h"
#include "broaditem/LayoutEngine.h"
#include "broaditem/LayoutRegistry.h"
#include "broaditem/Element.h"
#include <QDebug>

class TestParser : public QObject {
    Q_OBJECT

private slots:
    void testParseSimpleText();
    void testParseColumnWithChildren();
    void testParseDecorators();
    void testMeasureText();
    void testColumnMeasure();
    void testBindProperty();
    void testIfHas();
    void testRegistryLoad();
};

void TestParser::testParseSimpleText()
{
    QString xml = R"(
        <root>
            <text font-size="14">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
}

void TestParser::testParseColumnWithChildren()
{
    QString xml = R"(
        <root>
            <column space="4">
                <text>A</text>
                <text>B</text>
            </column>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
}

void TestParser::testParseDecorators()
{
    QString xml = R"(
        <root>
            <margin all="4">
                <border width="1" color="#555" radius="4">
                    <background color="#333" radius="3">
                        <padding all="8">
                            <text>设备状态</text>
                        </padding>
                    </background>
                </border>
            </margin>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
}

void TestParser::testMeasureText()
{
    QString xml = R"(
        <root>
            <text font-size="12">Test</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    BroadItem::LayoutContext ctx;
    BroadItem::LayoutConstraints constraints;
    auto result = root->measure(ctx, constraints);
    QVERIFY(result.intrinsicSize.width > 0);
    QVERIFY(result.intrinsicSize.height > 0);
}

void TestParser::testColumnMeasure()
{
    QString xml = R"(
        <root>
            <column space="4">
                <text font-size="12">Line1</text>
                <text font-size="12">Line2 longer</text>
            </column>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    BroadItem::LayoutContext ctx;
    BroadItem::LayoutConstraints constraints;
    auto result = root->measure(ctx, constraints);
    QVERIFY(result.intrinsicSize.width > 0);
    QVERIFY(result.intrinsicSize.height > 0);
}

void TestParser::testBindProperty()
{
    QString xml = R"(
        <root>
            <text bind="title">Default</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
    QVERIFY(root->bindsProperty("title"));
    QVERIFY(!root->bindsProperty("other"));
}

void TestParser::testIfHas()
{
    QString xml = R"(
        <root>
            <if-has bind="show">
                <text>Visible</text>
            </if-has>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    BroadItem::LayoutContext ctx;
    // Property not set -> should not show
    auto result = root->measure(ctx, BroadItem::LayoutConstraints{});
    QCOMPARE(result.intrinsicSize.width, 0.0);
    QCOMPARE(result.intrinsicSize.height, 0.0);

    ctx.dynamicProperties.insert("show", "yes");
    result = root->measure(ctx, BroadItem::LayoutConstraints{});
    QVERIFY(result.intrinsicSize.width > 0);
}

void TestParser::testRegistryLoad()
{
    int count = BroadItem::loadLayoutsFromDirectory("../tests");
    // Should load test_layout.xml
    QVERIFY(count >= 1);
}

QTEST_MAIN(TestParser)
#include "test_parser.moc"
