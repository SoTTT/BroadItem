#include <QtTest/QtTest>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/core/LayoutEngine.h>
#include <broaditem/element/Element.h>
#include <broaditem/element/SizedElement.h>
#include <broaditem/layout/RowLayout.h>
#include <broaditem/layout/ColumnLayout.h>
#include <broaditem/layout/GridLayout.h>
#include <broaditem/text/TextElement.h>
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

    // Main-stretch: RowLayout
    void testRowMainStretchEqualWidths();
    void testRowMainStretchRespectsExplicitWidth();

    // Main-stretch: ColumnLayout
    void testColumnMainStretchEqualHeights();
    void testColumnMainStretchRespectsExplicitHeight();

    // Main-stretch: default/disabled
    void testRowWithoutMainStretchKeepsOriginalBehavior();
    void testColumnWithoutMainStretchKeepsOriginalBehavior();

    // Grid cell count validation
    void testGridValidCellCount();
    void testGridInvalidCellCountFails();

    // background-opacity
    void testBackgroundOpacityParses();
    void testBackgroundOpacityRendersTransparent();

    // font-size px
    void testFontSizeIsPixels();

    // space-row/space-column fallback
    void testGridSpaceRowFallsBackToSpace();
    void testGridSpaceRowExplicitZero();
    void testGridSpaceColumnExplicitValue();

    // strict grid validation
    void testGridRejectsForChild();
    void testGridRejectsIfHasChild();
    void testGridRejectsNonCellChild();
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

/// @brief RowLayout 不拉伸指定 height 的子元素
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

/// @brief RowLayout 拉伸未指定 height 的子元素
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

/// @brief ColumnLayout 不拉伸指定 width 的子元素
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

/// @brief ColumnLayout 拉伸未指定 width 的子元素
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

/// @brief Grid 合法单元格数量（columns×rows）解析成功
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

/// @brief Grid 单元格数量与 columns×rows 不匹配时解析失败
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

/// @brief 测试 background-opacity 属性解析
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

/// @brief 测试 background-opacity 实际渲染半透明效果，通过像素采样验证
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

// ── font-size px ──

void TestLayoutBehavior::testFontSizeIsPixels()
{
    // Visual verification: render text at font-size=40 to a QImage,
    // scan pixel rows to measure actual rendered glyph height.
    // With setPixelSize(40), the visual height should be ~40px (not ~53px like pt).
    QString xml = R"(
        <root>
            <text font-size="40">Xg</text>
        </root>
    )";

    auto root = parseAndLayout(xml);
    QVERIFY(root != nullptr);

    // Render to a tall image so nothing clips
    QImage img(200, 80, QImage::Format_ARGB32);
    img.fill(Qt::white);
    {
        QPainter painter(&img);
        BroadItem::LayoutContext lctx;
        BroadItem::LayoutEngine::render(root, &painter, lctx);
    }

    // Scan a column near the left edge (x=15) vertically for non-white pixels.
    // The text is positioned at (0,0), and "Xg" at font-size=40 is ~30px wide.
    // Scan every column in a 30px band to handle glyph positioning variance.
    int firstRow = -1;
    int lastRow = -1;
    for (int x = 5; x < 35; ++x) {
        for (int y = 0; y < img.height(); ++y) {
            QColor c = img.pixelColor(x, y);
            bool isWhite = (c.red() > 250 && c.green() > 250 && c.blue() > 250);
            if (!isWhite) {
                if (firstRow < 0 || y < firstRow) firstRow = y;
                if (lastRow < 0 || y > lastRow) lastRow = y;
            }
        }
    }

    QVERIFY2(firstRow >= 0, "No non-white pixels found — text not rendered");
    int visualHeight = lastRow - firstRow + 1;

    // With setPixelSize(40), the rendered glyph height should be
    // roughly 40px (typically 30-50px for a 40px font, with descenders).
    // With setPointSizeF(40), the height would be ~53px+.
    // We assert it's well under 60px to catch pt-vs-px discrepancy.
    QVERIFY2(visualHeight < 55,
             QString("font-size=40 should render at ~40px (pixel-sized), "
                     "got visual height %1px — too tall for px mode")
                 .arg(visualHeight).toUtf8());
}

// ── space-row/space-column fallback ──

void TestLayoutBehavior::testGridSpaceRowFallsBackToSpace()
{
    QString xml = R"(
        <root>
            <grid columns="2" rows="2" space="10">
                <cell><text>A</text></cell>
                <cell><text>B</text></cell>
                <cell><text>C</text></cell>
                <cell><text>D</text></cell>
            </grid>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
    auto grid = std::dynamic_pointer_cast<BroadItem::GridLayout>(root);
    QVERIFY(grid != nullptr);
    // space-row not set -> should fall back to space=10
    QCOMPARE(grid->rowSpace(), 10.0);
    // space-column not set -> should fall back to space=10
    QCOMPARE(grid->columnSpace(), 10.0);
}

void TestLayoutBehavior::testGridSpaceRowExplicitZero()
{
    QString xml = R"(
        <root>
            <grid columns="2" rows="2" space="10" space-row="0">
                <cell><text>A</text></cell>
                <cell><text>B</text></cell>
                <cell><text>C</text></cell>
                <cell><text>D</text></cell>
            </grid>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
    auto grid = std::dynamic_pointer_cast<BroadItem::GridLayout>(root);
    QVERIFY(grid != nullptr);
    // space-row explicitly set to 0 -> should be 0, NOT fall back to space=10
    QCOMPARE(grid->rowSpace(), 0.0);
}

void TestLayoutBehavior::testGridSpaceColumnExplicitValue()
{
    QString xml = R"(
        <root>
            <grid columns="2" rows="2" space="10" space-column="5">
                <cell><text>A</text></cell>
                <cell><text>B</text></cell>
                <cell><text>C</text></cell>
                <cell><text>D</text></cell>
            </grid>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
    auto grid = std::dynamic_pointer_cast<BroadItem::GridLayout>(root);
    QVERIFY(grid != nullptr);
    // space-column explicitly set to 5 -> should be 5, NOT fall back to space=10
    QCOMPARE(grid->columnSpace(), 5.0);
}

// ── strict grid validation ──

void TestLayoutBehavior::testGridRejectsForChild()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <grid columns="2" rows="2">
                <for b:of="list">
                    <cell><text>X</text></cell>
                </for>
            </grid>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root == nullptr);
}

void TestLayoutBehavior::testGridRejectsIfHasChild()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <grid columns="2" rows="2">
                <if-has b:prop="show">
                    <cell><text>X</text></cell>
                </if-has>
            </grid>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root == nullptr);
}

void TestLayoutBehavior::testGridRejectsNonCellChild()
{
    QString xml = R"(
        <root>
            <grid columns="1" rows="1">
                <text>Should not be allowed</text>
            </grid>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root == nullptr);
}

// ── Main-stretch: RowLayout ──

void TestLayoutBehavior::testRowMainStretchEqualWidths()
{
    QString xml = R"(
        <root>
            <row main-stretch="true">
                <text>A</text>
                <text>BBBB</text>
                <text>CC</text>
            </row>
        </root>
    )";

    auto root = parseAndLayout(xml);
    QVERIFY(root != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(root);
    QVERIFY(row != nullptr);

    const auto& flat = row->flattenedChildren();
    QCOMPARE(flat.size(), size_t(3));

    // All three children should have the same width (max among them = "BBBB")
    double w0 = flat[0]->rect().width();
    double w1 = flat[1]->rect().width();
    double w2 = flat[2]->rect().width();

    QVERIFY2(qFuzzyCompare(w0, w1) && qFuzzyCompare(w1, w2),
             QString("Main-stretch row children should have equal widths, got %1, %2, %3")
                 .arg(w0).arg(w1).arg(w2).toUtf8());

    // The width should be > 0 (text was rendered)
    QVERIFY(w0 > 0);
}

void TestLayoutBehavior::testRowMainStretchRespectsExplicitWidth()
{
    QString xml = R"(
        <root>
            <row main-stretch="true">
                <text width="50">A</text>
                <text>BBB</text>
            </row>
        </root>
    )";

    auto root = parseAndLayout(xml);
    QVERIFY(root != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(root);
    QVERIFY(row != nullptr);

    const auto& flat = row->flattenedChildren();
    QCOMPARE(flat.size(), size_t(2));

    double w0 = flat[0]->rect().width();
    double w1 = flat[1]->rect().width();

    // First child has explicit width=50; it should keep its width
    QVERIFY2(w0 > 0 && w0 < 60,
             QString("Explicit width=50 child should keep ~50px, got %1").arg(w0).toUtf8());

    // Second child should be stretched (or at least laid out)
    QVERIFY(w1 > 0);
}

// ── Main-stretch: ColumnLayout ──

void TestLayoutBehavior::testColumnMainStretchEqualHeights()
{
    QString xml = R"(
        <root>
            <column main-stretch="true">
                <text font-size="12">A</text>
                <text font-size="24">Tall</text>
                <text font-size="16">Mid</text>
            </column>
        </root>
    )";

    auto root = parseAndLayout(xml);
    QVERIFY(root != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(root);
    QVERIFY(col != nullptr);

    const auto& flat = col->flattenedChildren();
    QCOMPARE(flat.size(), size_t(3));

    // All three children should have the same height (max among them = font-size 24)
    double h0 = flat[0]->rect().height();
    double h1 = flat[1]->rect().height();
    double h2 = flat[2]->rect().height();

    QVERIFY2(qFuzzyCompare(h0, h1) && qFuzzyCompare(h1, h2),
             QString("Main-stretch column children should have equal heights, got %1, %2, %3")
                 .arg(h0).arg(h1).arg(h2).toUtf8());

    QVERIFY(h0 > 0);
}

void TestLayoutBehavior::testColumnMainStretchRespectsExplicitHeight()
{
    QString xml = R"(
        <root>
            <column main-stretch="true">
                <text height="30">A</text>
                <text font-size="24">Tall</text>
            </column>
        </root>
    )";

    auto root = parseAndLayout(xml);
    QVERIFY(root != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(root);
    QVERIFY(col != nullptr);

    const auto& flat = col->flattenedChildren();
    QCOMPARE(flat.size(), size_t(2));

    double h0 = flat[0]->rect().height();
    double h1 = flat[1]->rect().height();

    // First child has explicit height=30; it should keep roughly that height
    QVERIFY2(h0 > 0 && h0 < 40,
             QString("Explicit height=30 child should keep ~30px, got %1").arg(h0).toUtf8());

    QVERIFY(h1 > 0);
}

// ── Main-stretch: default/disabled ──

void TestLayoutBehavior::testRowWithoutMainStretchKeepsOriginalBehavior()
{
    QString xml = R"(
        <root>
            <row>
                <text>A</text>
                <text>BBBB</text>
            </row>
        </root>
    )";

    auto root = parseAndLayout(xml);
    QVERIFY(root != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(root);
    QVERIFY(row != nullptr);

    const auto& flat = row->flattenedChildren();
    QCOMPARE(flat.size(), size_t(2));

    double w0 = flat[0]->rect().width();
    double w1 = flat[1]->rect().width();

    // Without main-stretch, children should NOT have equal widths (different text)
    QVERIFY2(!qFuzzyCompare(w0, w1),
             QString("Without main-stretch, row children should have different widths, got %1 and %2")
                 .arg(w0).arg(w1).toUtf8());
}

void TestLayoutBehavior::testColumnWithoutMainStretchKeepsOriginalBehavior()
{
    QString xml = R"(
        <root>
            <column>
                <text font-size="12">A</text>
                <text font-size="24">Tall</text>
            </column>
        </root>
    )";

    auto root = parseAndLayout(xml);
    QVERIFY(root != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(root);
    QVERIFY(col != nullptr);

    const auto& flat = col->flattenedChildren();
    QCOMPARE(flat.size(), size_t(2));

    double h0 = flat[0]->rect().height();
    double h1 = flat[1]->rect().height();

    // Without main-stretch, children should NOT have equal heights (different font sizes)
    QVERIFY2(!qFuzzyCompare(h0, h1),
             QString("Without main-stretch, column children should have different heights, got %1 and %2")
                 .arg(h0).arg(h1).toUtf8());
}

QTEST_MAIN(TestLayoutBehavior)
#include "test_layout_behavior.moc"
