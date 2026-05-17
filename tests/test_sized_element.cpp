#include <QtTest/QtTest>
#include "broaditem/XmlLayoutParser.h"
#include "broaditem/LayoutContext.h"
#include "broaditem/MapPropertyContext.h"
#include "broaditem/LayoutEngine.h"
#include "broaditem/Element.h"
#include "broaditem/SizedElement.h"
#include "broaditem/elements/TextElement.h"
#include "broaditem/elements/RowLayout.h"
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

/// @brief 解析带 width/height 属性的元素
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

/// @brief 测试固定宽高元素的测量结果与指定尺寸一致
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
    QCOMPARE(result.width(), 100.0);
    QCOMPARE(result.height(), 50.0);
}

/// @brief 测试未指定宽高时测量基于内容计算
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
    QVERIFY(result.width() > 0);
    QVERIFY(result.height() > 0);
}

/// @brief 测试 clone 保留 width/height 属性
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

/// @brief 测试指定尺寸的元素不会被 cross-align="stretch" 拉伸
void TestSizedElement::testLayoutStretch()
{
    // Test that width/height are hard constraints: layout does NOT stretch
    // elements that have explicitly specified sizes.
    QString xml = R"(
        <root>
            <row cross-align="stretch">
                <text width="100" height="50">Hello</text>
            </row>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    BroadItem::LayoutContext lctx;
    auto result = BroadItem::LayoutEngine::measure(root, lctx, BroadItem::LayoutConstraints{});
    // Layout into a rect larger than the text request size
    QRectF bigRect(0, 0, 300, 150);
    BroadItem::LayoutEngine::layout(root, lctx, bigRect);
    // Text has height=50; row has cross-align=stretch but should NOT stretch it
    // Verify text rect height is near 50 (not stretched to ~150)
    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(root);
    QVERIFY(row != nullptr);
    const auto& flat = row->flattenedChildren();
    QVERIFY(!flat.empty());
    auto text = std::dynamic_pointer_cast<BroadItem::TextElement>(flat[0]);
    QVERIFY(text != nullptr);
    double textHeight = text->rect().height();
    QVERIFY2(textHeight > 0 && textHeight < 100,
             QString("Text height should stay ~50 (not stretched), got %1").arg(textHeight).toUtf8());
}

QTEST_MAIN(TestSizedElement)
#include "test_sized_element.moc"
