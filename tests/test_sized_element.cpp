#include <QtTest/QtTest>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/core/LayoutEngine.h>
#include <broaditem/element/Element.h>
#include <broaditem/element/SizedElement.h>
#include <broaditem/element/text/TextElement.h>
#include <broaditem/element/layout/RowLayout.h>

using namespace BroadItem;

class TestSizedElement : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void testParseWidthHeight();
    void testMeasureWithSize();
    void testMeasureWithoutSize();
    void testParsePreservesSize();
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

    auto node = BroadItem::LayoutEngine::materialize(root, layoutCtx);
    QVERIFY(node != nullptr);
    auto result = BroadItem::LayoutEngine::measure(layoutCtx, constraints, *node);
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

    auto node = BroadItem::LayoutEngine::materialize(root, layoutCtx);
    QVERIFY(node != nullptr);
    auto result = BroadItem::LayoutEngine::measure(layoutCtx, constraints, *node);
    // 未指定 width/height 时，尺寸应基于内容计算
    QVERIFY(result.width() > 0);
    QVERIFY(result.height() > 0);
}

/// @brief 测试解析后 width/height 属性保留在模板元素上。
/// @details 原用例验证 clone() 保留尺寸属性；模板/实例分离后 Element 不可变、
/// 无 clone 路径（共享模板由 Registry/Frame 持有），等价保证转为：
/// 解析产物本身即携带 width/height，且物化/测量均直接读取模板。
void TestSizedElement::testParsePreservesSize()
{
    auto text = std::make_shared<BroadItem::TextElement>();
    // 模拟解析 width/height
    QDomDocument doc;
    auto elem = doc.createElement("text");
    elem.setAttribute("width", "100");
    elem.setAttribute("height", "50");
    text->parse(elem);

    QVERIFY(text != nullptr);
    QCOMPARE(text->width(), 100.0);
    QCOMPARE(text->height(), 50.0);
    QVERIFY(text->hasWidth());
    QVERIFY(text->hasHeight());
}

/// @brief 测试指定尺寸的元素不会被 cross-align="stretch" 拉伸
void TestSizedElement::testLayoutStretch()
{
    // 验证 width/height 是硬约束：布局不会拉伸
    // 显式指定了尺寸的元素。
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
    auto node = BroadItem::LayoutEngine::materialize(root, lctx);
    QVERIFY(node != nullptr);
    BroadItem::LayoutEngine::measure(lctx, BroadItem::LayoutConstraints{}, *node);
    // 以大于文本请求尺寸的矩形进行布局
    QRectF bigRect(0, 0, 300, 150);
    BroadItem::LayoutEngine::layout(lctx, bigRect, *node);
    // 文本 height=50；row 虽为 cross-align=stretch 但不应拉伸它
    // 校验文本矩形高度接近 50（未被拉伸到 ~150）
    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(root);
    QVERIFY(row != nullptr);
    QVERIFY(!node->children.empty());
    auto text = dynamic_cast<const BroadItem::TextElement*>(node->children[0]->element);
    QVERIFY(text != nullptr);
    double textHeight = node->children[0]->rect.height();
    QVERIFY2(textHeight > 0 && textHeight < 100,
             QString("文本高度应保持 ~50（未被拉伸），实际为 %1").arg(textHeight).toUtf8());
}

// NOLINTEND(readability-convert-member-functions-to-static)

QTEST_MAIN(TestSizedElement)
#include "test_sized_element.moc"
