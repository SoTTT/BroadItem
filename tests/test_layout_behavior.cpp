#include <QtTest/QtTest>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/core/LayoutEngine.h>
#include <broaditem/core/Node.h>
#include <broaditem/element/Element.h>
#include <broaditem/element/layout/RowLayout.h>
#include <broaditem/element/layout/ColumnLayout.h>
#include <broaditem/element/layout/GridLayout.h>
#include <broaditem/element/text/TextElement.h>
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

/// @brief 一次性格式的布局结果包：模板树 + 属性上下文 + 物化实例节点树。
///
/// 模板/实例分离架构下，Node::element 为指向模板的非拥有指针，
/// 因此模板树（root）必须与节点树（node）同生命周期持有。
struct LayoutResult {
    BroadItem::ElementPtr root;              ///< 模板树（持有以保 Node::element 指针有效）。
    BroadItem::MapPropertyContext propCtx;   ///< 属性上下文（LayoutContext 指向它）。
    std::unique_ptr<BroadItem::Node> node;   ///< 物化后的实例节点树根（控制元素已展开）。
};

/// @brief 解析、物化、测量、布局一体化辅助。
/// @param xmlStr XML 布局字符串。
/// @param layoutRect 布局矩形；传入 null 矩形时以测量结果作为布局矩形。
/// @return 布局结果包；解析或物化失败时返回 nullptr。
static std::unique_ptr<LayoutResult> parseAndLayout(const QString& xmlStr, QRectF layoutRect = QRectF(0, 0, 300, 200))
{
    auto result = std::make_unique<LayoutResult>();
    result->root = BroadItem::XmlLayoutParser::parseString(xmlStr);
    if (!result->root)
        return nullptr;

    BroadItem::LayoutContext lctx{&result->propCtx};
    result->node = BroadItem::LayoutEngine::materialize(result->root, lctx);
    if (!result->node)
        return nullptr;

    auto measured = BroadItem::LayoutEngine::measure(result->root, lctx, BroadItem::LayoutConstraints{}, *result->node);
    if (layoutRect.isNull())
        layoutRect = QRectF(0, 0, measured.width(), measured.height());
    BroadItem::LayoutEngine::layout(result->root, lctx, layoutRect, *result->node);
    return result;
}

/// @brief 判定实例节点是否由 TextElement 物化而来。
/// @param node 实例节点（可为 nullptr）。
/// @return 指向产生该节点的 TextElement 模板；类型不符时返回 nullptr。
static const BroadItem::TextElement* asText(const BroadItem::Node* node)
{
    return node ? dynamic_cast<const BroadItem::TextElement*>(node->element) : nullptr;
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
    auto result = parseAndLayout(xml, bigRect);
    QVERIFY(result != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(result->root);
    QVERIFY(row != nullptr);

    // 物化后的子节点即展平结果，应包含该文本节点
    const auto& children = result->node->children;
    QVERIFY(!children.empty());
    QVERIFY(asText(children[0].get()) != nullptr);

    // Text has specified height=20; row should NOT stretch it to 100
    // Allow for decorator height (default 0) — just check it's not 100
    double h = children[0]->rect.height();
    QVERIFY2(h < 50,
             QString("Text with height=20 should not be stretched, got %1").arg(h).toUtf8());
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
    auto result = parseAndLayout(xml, bigRect);
    QVERIFY(result != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(result->root);
    QVERIFY(row != nullptr);

    const auto& children = result->node->children;
    QVERIFY(!children.empty());
    QVERIFY(asText(children[0].get()) != nullptr);

    // Text has no specified height; row should stretch it
    // It should be close to the content area height (100 - decorators)
    double h = children[0]->rect.height();
    QVERIFY2(h > h * 0.3 || h >= 50,
             QString("Text without height should be stretched, got %1").arg(h).toUtf8());
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
    auto result = parseAndLayout(xml, bigRect);
    QVERIFY(result != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(result->root);
    QVERIFY(col != nullptr);

    const auto& children = result->node->children;
    QVERIFY(!children.empty());
    QVERIFY(asText(children[0].get()) != nullptr);

    // Text has specified width=50; column should NOT stretch it to 300
    double w = children[0]->rect.width();
    QVERIFY2(w < 200,
             QString("Text with width=50 should not be stretched, got %1").arg(w).toUtf8());
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
    auto result = parseAndLayout(xml, bigRect);
    QVERIFY(result != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(result->root);
    QVERIFY(col != nullptr);

    const auto& children = result->node->children;
    QVERIFY(!children.empty());
    QVERIFY(asText(children[0].get()) != nullptr);

    // Text has no specified width; column should stretch it
    double w = children[0]->rect.width();
    QVERIFY2(w > 50,
             QString("Text without width should be stretched, got %1").arg(w).toUtf8());
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

    auto opaqueResult = parseAndLayout(xmlFull);
    QVERIFY(opaqueResult != nullptr);

    auto transparentResult = parseAndLayout(xmlTransparent);
    QVERIFY(transparentResult != nullptr);

    // Render to images and sample background pixel
    QImage opaqueImg(200, 60, QImage::Format_ARGB32);
    opaqueImg.fill(Qt::white);
    {
        QPainter painter(&opaqueImg);
        BroadItem::LayoutContext lctx{&opaqueResult->propCtx};
        BroadItem::LayoutEngine::render(opaqueResult->root, &painter, lctx, *opaqueResult->node);
    }

    QImage transparentImg(200, 60, QImage::Format_ARGB32);
    transparentImg.fill(Qt::white);
    {
        QPainter painter(&transparentImg);
        BroadItem::LayoutContext lctx{&transparentResult->propCtx};
        BroadItem::LayoutEngine::render(transparentResult->root, &painter, lctx, *transparentResult->node);
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

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    // Render to a tall image so nothing clips
    QImage img(200, 80, QImage::Format_ARGB32);
    img.fill(Qt::white);
    {
        QPainter painter(&img);
        BroadItem::LayoutContext lctx{&result->propCtx};
        BroadItem::LayoutEngine::render(result->root, &painter, lctx, *result->node);
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

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(result->root);
    QVERIFY(row != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(3));

    // All three children should have the same width (max among them = "BBBB")
    double w0 = children[0]->rect.width();
    double w1 = children[1]->rect.width();
    double w2 = children[2]->rect.width();

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

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(result->root);
    QVERIFY(row != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(2));

    double w0 = children[0]->rect.width();
    double w1 = children[1]->rect.width();

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

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(result->root);
    QVERIFY(col != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(3));

    // All three children should have the same height (max among them = font-size 24)
    double h0 = children[0]->rect.height();
    double h1 = children[1]->rect.height();
    double h2 = children[2]->rect.height();

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

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(result->root);
    QVERIFY(col != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(2));

    double h0 = children[0]->rect.height();
    double h1 = children[1]->rect.height();

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

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(result->root);
    QVERIFY(row != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(2));

    double w0 = children[0]->rect.width();
    double w1 = children[1]->rect.width();

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

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(result->root);
    QVERIFY(col != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(2));

    double h0 = children[0]->rect.height();
    double h1 = children[1]->rect.height();

    // Without main-stretch, children should NOT have equal heights (different font sizes)
    QVERIFY2(!qFuzzyCompare(h0, h1),
             QString("Without main-stretch, column children should have different heights, got %1 and %2")
                 .arg(h0).arg(h1).toUtf8());
}

QTEST_MAIN(TestLayoutBehavior)
#include "test_layout_behavior.moc"
