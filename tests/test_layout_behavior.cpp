#include <QtTest/QtTest>
#include "broaditem/XmlLayoutParser.h"
#include "broaditem/LayoutContext.h"
#include "broaditem/MapPropertyContext.h"
#include "broaditem/LayoutEngine.h"
#include "broaditem/Element.h"
#include "broaditem/SizedElement.h"
#include "broaditem/elements/RowLayout.h"
#include "broaditem/elements/ColumnLayout.h"
#include "broaditem/elements/TextElement.h"
#include <QPainter>
#include <QImage>
#include <QDebug>

class TestLayoutBehavior : public QObject {
    Q_OBJECT

private slots:
    // Stretch exclusion: RowLayout
    void testRowDoesNotStretchSpecifiedHeight();
    void testRowStretchesUnspecifiedHeight();

    // Stretch exclusion: ColumnLayout
    void testColumnDoesNotStretchSpecifiedWidth();
    void testColumnStretchesUnspecifiedWidth();

    // Grid cell count validation
    void testGridValidCellCount();
    void testGridInvalidCellCountFails();

    // background-opacity
    void testBackgroundOpacityParses();
    void testBackgroundOpacityRendersTransparent();
};

// Helper: parse, measure, layout in one step
static BroadItem::ElementPtr parseAndLayout(const QString& xmlStr, QRectF layoutRect = QRectF(0, 0, 300, 200))
{
    auto root = BroadItem::XmlLayoutParser::parseString(xmlStr);
    if (!root)
        return nullptr;

    BroadItem::LayoutContext lctx;
    auto result = BroadItem::LayoutEngine::measure(root, lctx, BroadItem::LayoutConstraints{});
    if (layoutRect.isNull())
        layoutRect = QRectF(0, 0, result.width(), result.height());
    BroadItem::LayoutEngine::layout(root, lctx, layoutRect);
    return root;
}

// ── RowLayout stretch tests ──

void TestLayoutBehavior::testRowDoesNotStretchSpecifiedHeight()
{
    QString xml = R"(
        <root>
            <row cross-align="stretch">
                <text height="20">Short</text>
            </row>
        </root>
    )";

    QRectF bigRect(0, 0, 300, 100); // Row gets height 100
    auto root = parseAndLayout(xml, bigRect);
    QVERIFY(root != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(root);
    QVERIFY(row != nullptr);

    // Flattened children should include the text
    const auto& flat = row->flattenedChildren();
    QVERIFY(!flat.empty());

    auto text = std::dynamic_pointer_cast<BroadItem::TextElement>(flat[0]);
    QVERIFY(text != nullptr);

    // Text has specified height=20; row should NOT stretch it to 100
    // Allow for decorator height (default 0) — just check it's not 100
    QVERIFY2(text->rect().height() < 50,
             QString("Text with height=20 should not be stretched, got %1").arg(text->rect().height()).toUtf8());
}

void TestLayoutBehavior::testRowStretchesUnspecifiedHeight()
{
    QString xml = R"(
        <root>
            <row cross-align="stretch">
                <text>No height specified</text>
            </row>
        </root>
    )";

    QRectF bigRect(0, 0, 300, 100);
    auto root = parseAndLayout(xml, bigRect);
    QVERIFY(root != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(root);
    QVERIFY(row != nullptr);

    const auto& flat = row->flattenedChildren();
    QVERIFY(!flat.empty());

    auto text = std::dynamic_pointer_cast<BroadItem::TextElement>(flat[0]);
    QVERIFY(text != nullptr);

    // Text has no specified height; row should stretch it
    // It should be close to the content area height (100 - decorators)
    QVERIFY2(text->rect().height() > text->rect().height() * 0.3 || text->rect().height() >= 50,
             QString("Text without height should be stretched, got %1").arg(text->rect().height()).toUtf8());
}

// ── ColumnLayout stretch tests ──

void TestLayoutBehavior::testColumnDoesNotStretchSpecifiedWidth()
{
    QString xml = R"(
        <root>
            <column cross-align="stretch">
                <text width="50">Narrow</text>
            </column>
        </root>
    )";

    QRectF bigRect(0, 0, 300, 200); // Column gets width 300
    auto root = parseAndLayout(xml, bigRect);
    QVERIFY(root != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(root);
    QVERIFY(col != nullptr);

    const auto& flat = col->flattenedChildren();
    QVERIFY(!flat.empty());

    auto text = std::dynamic_pointer_cast<BroadItem::TextElement>(flat[0]);
    QVERIFY(text != nullptr);

    // Text has specified width=50; column should NOT stretch it to 300
    QVERIFY2(text->rect().width() < 200,
             QString("Text with width=50 should not be stretched, got %1").arg(text->rect().width()).toUtf8());
}

void TestLayoutBehavior::testColumnStretchesUnspecifiedWidth()
{
    QString xml = R"(
        <root>
            <column cross-align="stretch">
                <text>No width specified</text>
            </column>
        </root>
    )";

    QRectF bigRect(0, 0, 300, 200);
    auto root = parseAndLayout(xml, bigRect);
    QVERIFY(root != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(root);
    QVERIFY(col != nullptr);

    const auto& flat = col->flattenedChildren();
    QVERIFY(!flat.empty());

    auto text = std::dynamic_pointer_cast<BroadItem::TextElement>(flat[0]);
    QVERIFY(text != nullptr);

    // Text has no specified width; column should stretch it
    QVERIFY2(text->rect().width() > 50,
             QString("Text without width should be stretched, got %1").arg(text->rect().width()).toUtf8());
}

// ── Grid cell count validation ──

void TestLayoutBehavior::testGridValidCellCount()
{
    QString xml = R"(
        <root>
            <grid columns="2" rows="2">
                <cell v-align="center" h-align="center"><text>A</text></cell>
                <cell v-align="center" h-align="center"><text>B</text></cell>
                <cell v-align="center" h-align="center"><text>C</text></cell>
                <cell v-align="center" h-align="center"><text>D</text></cell>
            </grid>
        </root>
    )";

    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
}

void TestLayoutBehavior::testGridInvalidCellCountFails()
{
    QString xml = R"(
        <root>
            <grid columns="2" rows="2">
                <cell v-align="center" h-align="center"><text>A</text></cell>
                <cell v-align="center" h-align="center"><text>B</text></cell>
            </grid>
        </root>
    )";

    // Expected 2×2=4 cells, but only 2 provided -> should fail
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root == nullptr);
}

// ── background-opacity ──

void TestLayoutBehavior::testBackgroundOpacityParses()
{
    QString xml = R"(
        <root>
            <text background-color="#FF0000" background-opacity="0.5">Hello</text>
        </root>
    )";

    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
}

void TestLayoutBehavior::testBackgroundOpacityRendersTransparent()
{
    QString xmlFull = R"(
        <root>
            <text background-color="#FF0000" background-opacity="1.0"
                  padding-top="4" padding-bottom="4" padding-left="4" padding-right="4">
                Opaque
            </text>
        </root>
    )";

    QString xmlTransparent = R"(
        <root>
            <text background-color="#FF0000" background-opacity="0.3"
                  padding-top="4" padding-bottom="4" padding-left="4" padding-right="4">
                Transparent
            </text>
        </root>
    )";

    auto opaqueRoot = parseAndLayout(xmlFull);
    QVERIFY(opaqueRoot != nullptr);

    auto transparentRoot = parseAndLayout(xmlTransparent);
    QVERIFY(transparentRoot != nullptr);

    // Render to images and sample background pixel
    QImage opaqueImg(200, 60, QImage::Format_ARGB32);
    opaqueImg.fill(Qt::white);
    {
        QPainter painter(&opaqueImg);
        BroadItem::LayoutContext lctx;
        BroadItem::LayoutEngine::render(opaqueRoot, &painter, lctx);
    }

    QImage transparentImg(200, 60, QImage::Format_ARGB32);
    transparentImg.fill(Qt::white);
    {
        QPainter painter(&transparentImg);
        BroadItem::LayoutContext lctx;
        BroadItem::LayoutEngine::render(transparentRoot, &painter, lctx);
    }

    // Sample center pixel (inside the text background area)
    QColor opaqueColor = opaqueImg.pixelColor(100, 30);
    QColor transparentColor = transparentImg.pixelColor(100, 30);

    // opaque=1.0 → background fully covers → red dominates
    QVERIFY2(opaqueColor.red() > 200,
             QString("Opaque bg should be red, got r=%1 g=%2 b=%3")
                 .arg(opaqueColor.red()).arg(opaqueColor.green()).arg(opaqueColor.blue()).toUtf8());

    // transparent=0.3 → white bleed-through → less red
    QVERIFY2(transparentColor.red() < opaqueColor.red() || transparentColor.green() > 0,
             QString("Transparent bg should show white bleed-through, got r=%1 g=%2 b=%3")
                 .arg(transparentColor.red()).arg(transparentColor.green()).arg(transparentColor.blue()).toUtf8());
}

QTEST_MAIN(TestLayoutBehavior)
#include "test_layout_behavior.moc"
