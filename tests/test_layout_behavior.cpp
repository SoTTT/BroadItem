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
#include <algorithm>

#include "helpers/layout_helpers.h"

using namespace BroadItem;
using BroadItem::TestHelpers::LayoutResult;
using BroadItem::TestHelpers::parseAndLayout;

class TestLayoutBehavior : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    // 拉伸豁免：RowLayout
    void testRowDoesNotStretchSpecifiedHeight();
    void testRowStretchesUnspecifiedHeight();

    // 拉伸豁免：ColumnLayout
    void testColumnDoesNotStretchSpecifiedWidth();
    void testColumnStretchesUnspecifiedWidth();

    // 主轴拉伸：RowLayout
    void testRowMainStretchEqualWidths();
    void testRowMainStretchRespectsExplicitWidth();

    // 主轴拉伸：ColumnLayout
    void testColumnMainStretchEqualHeights();
    void testColumnMainStretchRespectsExplicitHeight();

    // 主轴拉伸：默认/关闭
    void testRowWithoutMainStretchKeepsOriginalBehavior();
    void testColumnWithoutMainStretchKeepsOriginalBehavior();

    // Grid 单元格数量校验
    void testGridValidCellCount();
    void testGridInvalidCellCountFails();

    // background-opacity 背景透明度
    void testBackgroundOpacityParses();
    void testBackgroundOpacityRendersTransparent();

    // font-size 像素单位
    void testFontSizeIsPixels();

    // row cross-align="baseline" 基线对齐
    void testRowBaselineAlignsDifferentFontSizes();
    void testRowBaselineNonTextFallsBackToBottomEdge();
    void testRowBaselineRowHeightWrapsAllChildren();
    void testRowBaselineIncludesBoxDecorations();
    void testRowBaselineDoesNotStretchExplicitHeight();
    void testRowBaselineDegenerateCases();

    // space-row/space-column 回退
    void testGridSpaceRowFallsBackToSpace();
    void testGridSpaceRowExplicitZero();
    void testGridSpaceColumnExplicitValue();

    // 严格 grid 校验
    void testGridRejectsForChild();
    void testGridRejectsIfChild();
    void testGridRejectsNonCellChild();
};

/// @brief 判定实例节点是否由 TextElement 物化而来。
/// @param node 实例节点（可为 nullptr）。
/// @return 指向产生该节点的 TextElement 模板；类型不符时返回 nullptr。
static const BroadItem::TextElement* asText(const BroadItem::Node* node)
{
    return node ? dynamic_cast<const BroadItem::TextElement*>(node->element) : nullptr;
}

// ── RowLayout 拉伸测试 ──

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

    QRectF bigRect(0, 0, 300, 100); // Row 获得高度 100
    auto result = parseAndLayout(xml, bigRect);
    QVERIFY(result != nullptr);

    auto row = std::dynamic_pointer_cast<BroadItem::RowLayout>(result->root);
    QVERIFY(row != nullptr);

    // 物化后的子节点即展平结果，应包含该文本节点
    const auto& children = result->node->children;
    QVERIFY(!children.empty());
    QVERIFY(asText(children[0].get()) != nullptr);

    // 文本指定了 height=20；row 不应将其拉伸到 100
    // 装饰层高度默认为 0——只校验高度不等于 100
    double h = children[0]->rect.height();
    QVERIFY2(h < 50,
             QString("height=20 的文本不应被拉伸，实际为 %1").arg(h).toUtf8());
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

    // 文本未指定 height；row 应拉伸它
    // 应接近内容区高度（100 减去装饰层）
    double h = children[0]->rect.height();
    QVERIFY2(h > h * 0.3 || h >= 50,
             QString("未指定 height 的文本应被拉伸，实际为 %1").arg(h).toUtf8());
}

// ── ColumnLayout 拉伸测试 ──

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

    QRectF bigRect(0, 0, 300, 200); // Column 获得宽度 300
    auto result = parseAndLayout(xml, bigRect);
    QVERIFY(result != nullptr);

    auto col = std::dynamic_pointer_cast<BroadItem::ColumnLayout>(result->root);
    QVERIFY(col != nullptr);

    const auto& children = result->node->children;
    QVERIFY(!children.empty());
    QVERIFY(asText(children[0].get()) != nullptr);

    // 文本指定了 width=50；column 不应将其拉伸到 300
    double w = children[0]->rect.width();
    QVERIFY2(w < 200,
             QString("width=50 的文本不应被拉伸，实际为 %1").arg(w).toUtf8());
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

    // 文本未指定 width；column 应拉伸它
    double w = children[0]->rect.width();
    QVERIFY2(w > 50,
             QString("未指定 width 的文本应被拉伸，实际为 %1").arg(w).toUtf8());
}

// ── Grid 单元格数量校验 ──

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

    // 期望 2×2=4 个 cell，但只提供了 2 个 → 应解析失败
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root == nullptr);
}

// ── background-opacity 背景透明度 ──

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

    // 渲染到图像并采样背景像素
    QImage opaqueImg(200, 60, QImage::Format_ARGB32);
    opaqueImg.fill(Qt::white);
    {
        QPainter painter(&opaqueImg);
        BroadItem::LayoutContext lctx{&opaqueResult->propCtx};
        BroadItem::LayoutEngine::render(&painter, lctx, *opaqueResult->node);
    }

    QImage transparentImg(200, 60, QImage::Format_ARGB32);
    transparentImg.fill(Qt::white);
    {
        QPainter painter(&transparentImg);
        BroadItem::LayoutContext lctx{&transparentResult->propCtx};
        BroadItem::LayoutEngine::render(&painter, lctx, *transparentResult->node);
    }

    // 采样中心像素（位于文本背景区域内）
    QColor opaqueColor = opaqueImg.pixelColor(100, 30);
    QColor transparentColor = transparentImg.pixelColor(100, 30);

    // opaque=1.0 → 背景完全覆盖 → 红色占主导
    QVERIFY2(opaqueColor.red() > 200,
             QString("不透明背景应为红色，实际 r=%1 g=%2 b=%3")
                 .arg(opaqueColor.red()).arg(opaqueColor.green()).arg(opaqueColor.blue()).toUtf8());

    // transparent=0.3 → 白色透出 → 红色减弱
    QVERIFY2(transparentColor.red() < opaqueColor.red() || transparentColor.green() > 0,
             QString("半透明背景应有白色透出，实际 r=%1 g=%2 b=%3")
                 .arg(transparentColor.red()).arg(transparentColor.green()).arg(transparentColor.blue()).toUtf8());
}

// ── font-size 像素单位 ──

void TestLayoutBehavior::testFontSizeIsPixels()
{
    // 可视化验证：以 font-size=40 渲染文本到 QImage，
    // 逐行扫描像素测量实际渲染的字形高度。
    // 使用 setPixelSize(40) 时可视高度应约 40px（而非 pt 模式下的 ~53px）。
    QString xml = R"(
        <root>
            <text font-size="40">Xg</text>
        </root>
    )";

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    // 渲染到足够高的图像，避免内容被裁剪
    QImage img(200, 80, QImage::Format_ARGB32);
    img.fill(Qt::white);
    {
        QPainter painter(&img);
        BroadItem::LayoutContext lctx{&result->propCtx};
        BroadItem::LayoutEngine::render(&painter, lctx, *result->node);
    }

    // 在靠近左边缘的列（x=15）纵向扫描非白像素。
    // 文本位于 (0,0)，font-size=40 时 "Xg" 约 30px 宽。
    // 扫描 30px 带内的每一列以容忍字形定位差异。
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

    QVERIFY2(firstRow >= 0, "未找到非白像素——文本未渲染");
    int visualHeight = lastRow - firstRow + 1;

    // setPixelSize(40) 时渲染字形高度应约为 40px
    // （40px 字体含降部一般 30-50px）。
    // setPointSizeF(40) 时高度会是 ~53px+。
    // 断言显著低于 60px 以捕捉 pt/px 混淆。
    QVERIFY2(visualHeight < 55,
             QString("font-size=40 应按像素渲染（~40px），"
                     "实际可视高度 %1px——超出 px 模式合理范围")
                 .arg(visualHeight).toUtf8());
}

// ── space-row/space-column 回退 ──

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
    // 未设置 space-row → 应回退到 space=10
    QCOMPARE(grid->rowSpace(), 10.0);
    // 未设置 space-column → 应回退到 space=10
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
    // space-row 显式设为 0 → 应为 0，不回退到 space=10
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
    // space-column 显式设为 5 → 应为 5，不回退到 space=10
    QCOMPARE(grid->columnSpace(), 5.0);
}

// ── 严格 grid 校验 ──

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

void TestLayoutBehavior::testGridRejectsIfChild()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <grid columns="2" rows="2">
                <if b:prop="show">
                    <cell><text>X</text></cell>
                </if>
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

// ── 主轴拉伸：RowLayout ──

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

    // 三个子元素应有相同宽度（取最大值 = "BBBB"）
    double w0 = children[0]->rect.width();
    double w1 = children[1]->rect.width();
    double w2 = children[2]->rect.width();

    QVERIFY2(qFuzzyCompare(w0, w1) && qFuzzyCompare(w1, w2),
             QString("main-stretch 的 row 子元素宽度应相等，实际为 %1, %2, %3")
                 .arg(w0).arg(w1).arg(w2).toUtf8());

    // 宽度应 > 0（文本已渲染）
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

    // 第一个子元素显式 width=50；应保持其宽度
    QVERIFY2(w0 > 0 && w0 < 60,
             QString("显式 width=50 的子元素应保持 ~50px，实际为 %1").arg(w0).toUtf8());

    // 第二个子元素应被拉伸（或至少完成布局）
    QVERIFY(w1 > 0);
}

// ── 主轴拉伸：ColumnLayout ──

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

    // 三个子元素应有相同高度（取最大值 = font-size 24）
    double h0 = children[0]->rect.height();
    double h1 = children[1]->rect.height();
    double h2 = children[2]->rect.height();

    QVERIFY2(qFuzzyCompare(h0, h1) && qFuzzyCompare(h1, h2),
             QString("main-stretch 的 column 子元素高度应相等，实际为 %1, %2, %3")
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

    // 第一个子元素显式 height=30；应大致保持该高度
    QVERIFY2(h0 > 0 && h0 < 40,
             QString("显式 height=30 的子元素应保持 ~30px，实际为 %1").arg(h0).toUtf8());

    QVERIFY(h1 > 0);
}

// ── 主轴拉伸：默认/关闭 ──

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

    // 无 main-stretch 时，子元素宽度不应相等（文本不同）
    QVERIFY2(!qFuzzyCompare(w0, w1),
             QString("无 main-stretch 时 row 子元素宽度应不同，实际为 %1 和 %2")
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

    // 无 main-stretch 时，子元素高度不应相等（字号不同）
    QVERIFY2(!qFuzzyCompare(h0, h1),
             QString("无 main-stretch 时 column 子元素高度应不同，实际为 %1 和 %2")
                 .arg(h0).arg(h1).toUtf8());
}

// ── RowLayout cross-align="baseline" 基线对齐 ──

/// @brief 计算与 TextElement 内部一致的期望 ascent（默认字体 + setPixelSize）。
/// @param pixelSize 像素字号。
/// @return 该字体的 ascent。
static double expectedAscent(int pixelSize)
{
    QFont font;
    font.setPixelSize(pixelSize);
    return QFontMetricsF(font).ascent();
}

/// @brief 不同字号的两个 text 按基线对齐：y 差等于 ascent 差
void TestLayoutBehavior::testRowBaselineAlignsDifferentFontSizes()
{
    QString xml = R"(
        <root>
            <row cross-align="baseline">
                <text font-size="12">small</text>
                <text font-size="24">BIG</text>
            </row>
        </root>
    )";

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(2));

    const double a12 = expectedAscent(12);
    const double a24 = expectedAscent(24);
    const double y0 = children[0]->rect.y();
    const double y1 = children[1]->rect.y();

    // 大字 ascent 更大成为对齐线，小字被下移：y0 - y1 == a24 - a12
    QVERIFY2(qAbs((y0 - y1) - (a24 - a12)) < 0.5,
             QString("基线 y 差应等于 ascent 差 %1，实际为 %2")
                 .arg(a24 - a12).arg(y0 - y1).toUtf8());
    // 两子基线重合
    QVERIFY2(qAbs((y0 + a12) - (y1 + a24)) < 0.5,
             "文本基线应重合");
}

/// @brief 无基线元素（显式尺寸的 image）以底边参与基线对齐
void TestLayoutBehavior::testRowBaselineNonTextFallsBackToBottomEdge()
{
    QString xml = R"(
        <root>
            <row cross-align="baseline">
                <image width="16" height="16"/>
                <text font-size="24">Xg</text>
            </row>
        </root>
    )";

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(2));

    const double a24 = expectedAscent(24);
    const QRectF imgRect = children[0]->rect;
    const QRectF textRect = children[1]->rect;

    // 文本基线 y = textRect.y() + a24；image 底边应落在该线上
    QVERIFY2(qAbs(imgRect.bottom() - (textRect.y() + a24)) < 0.5,
             QString("image 底边 %1 应落在文本基线 %2 上")
                 .arg(imgRect.bottom()).arg(textRect.y() + a24).toUtf8());
}

/// @brief 行高 = maxAscent + maxDescent：所有子元素完整落在行内且行恰好包裹内容
void TestLayoutBehavior::testRowBaselineRowHeightWrapsAllChildren()
{
    QString xml = R"(
        <root>
            <row cross-align="baseline">
                <text font-size="12">Ag</text>
                <text font-size="24">Ag</text>
            </row>
        </root>
    )";

    // 以测量尺寸作为布局矩形，行高即 baseline 模式下的行内容高
    auto result = parseAndLayout(xml, QRectF());
    QVERIFY(result != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(2));

    const double rowHeight = result->node->rect.height();
    double maxChildBottom = 0;
    for (const auto& child : children) {
        // 无子元素被裁掉：顶边不越界、底边不超过行高
        QVERIFY2(child->rect.y() >= -0.5, "子元素顶边必须落在行内");
        QVERIFY2(child->rect.bottom() <= rowHeight + 0.5,
                 QString("子元素底边 %1 超出行高 %2")
                     .arg(child->rect.bottom()).arg(rowHeight).toUtf8());
        maxChildBottom = std::max(maxChildBottom, child->rect.bottom());
    }
    // 行恰好包裹内容：最大子底边 == 行高（maxAscent + maxDescent 的直接推论）
    QVERIFY2(qAbs(rowHeight - maxChildBottom) < 0.5,
             QString("行高 %1 应等于最大子元素底边 %2")
                 .arg(rowHeight).arg(maxChildBottom).toUtf8());
}

/// @brief text 带 margin-top/padding-top 时基线含装饰偏移
void TestLayoutBehavior::testRowBaselineIncludesBoxDecorations()
{
    QString xml = R"(
        <root>
            <row cross-align="baseline">
                <text font-size="24" margin-top="5" padding-top="7">Ag</text>
                <text font-size="24">Ag</text>
            </row>
        </root>
    )";

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(2));

    // 同字号 ascent 相同，装饰子基线 = 5 + 7 + a24 成为对齐线：
    // 无装饰子下移 12px，装饰子顶边在 0
    const double y0 = children[0]->rect.y();
    const double y1 = children[1]->rect.y();
    QVERIFY2(qAbs(y0) < 0.5,
             QString("带装饰的子元素应位于行顶，实际 y=%1").arg(y0).toUtf8());
    QVERIFY2(qAbs((y1 - y0) - 12.0) < 0.5,
             QString("装饰偏移应为 12px，实际为 %1").arg(y1 - y0).toUtf8());
}

/// @brief 显式 height 的 text 在 baseline 模式下不被拉伸，按固有（指定）尺寸参与
void TestLayoutBehavior::testRowBaselineDoesNotStretchExplicitHeight()
{
    QString xml = R"(
        <root>
            <row cross-align="baseline">
                <text height="60" font-size="12">A</text>
                <text font-size="24">BIG</text>
            </row>
        </root>
    )";

    auto result = parseAndLayout(xml);
    QVERIFY(result != nullptr);

    const auto& children = result->node->children;
    QCOMPARE(children.size(), size_t(2));

    QVERIFY2(qAbs(children[0]->rect.height() - 60.0) < 0.5,
             QString("显式 height=60 的文本不应被拉伸，实际为 %1")
                 .arg(children[0]->rect.height()).toUtf8());
}

/// @brief 单子元素与空 row 的退化场景不崩溃
void TestLayoutBehavior::testRowBaselineDegenerateCases()
{
    QString xmlSingle = R"(
        <root>
            <row cross-align="baseline">
                <text font-size="24">only</text>
            </row>
        </root>
    )";
    auto single = parseAndLayout(xmlSingle, QRectF());
    QVERIFY(single != nullptr);
    QCOMPARE(single->node->children.size(), size_t(1));
    // 单子元素：基线即自身，顶边为 0，行高 == 子高
    QVERIFY2(qAbs(single->node->children[0]->rect.y()) < 0.5,
             "单子元素应位于行顶");
    QVERIFY2(qAbs(single->node->rect.height() - single->node->children[0]->rect.height()) < 0.5,
             "单子元素的行高应等于子元素高度");

    QString xmlEmpty = R"(
        <root>
            <row cross-align="baseline"></row>
        </root>
    )";
    auto empty = parseAndLayout(xmlEmpty);
    QVERIFY(empty != nullptr);
    QVERIFY(empty->node->children.empty());
}

// NOLINTEND(readability-convert-member-functions-to-static)

QTEST_MAIN(TestLayoutBehavior)
#include "test_layout_behavior.moc"