#include <QtTest/QtTest>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/core/LayoutEngine.h>
#include <broaditem/core/Node.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/element/Element.h>
#include <broaditem/element/rect/RectElement.h>

#include "helpers/binding_helpers.h"
#include "helpers/diagnostics_helpers.h"
#include "helpers/layout_helpers.h"

using namespace BroadItem;
using namespace BroadItem::TestHelpers;

/// @brief 判定实例节点是否由 RectElement 物化而来。
static const RectElement* asRect(const Node* node)
{
    return node ? dynamic_cast<const RectElement*>(node->element) : nullptr;
}

class TestRectElement : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    /// @brief 用例1：measure 逐维度规则——全显式 / 单维显式 / 全缺省（0×0），
    ///        盒模型装饰计入固有尺寸。
    void measureVariantsAndBoxModel()
    {
        // 全显式：作者声明即固有尺寸
        auto full = parseAndLayout(wrap(QStringLiteral(
            "<rect width=\"40\" height=\"2\"/>")), QRectF());
        QVERIFY(full != nullptr);
        QCOMPARE(full->node->rect.size(), QSizeF(40, 2));

        // 单维显式：未给维度取 0
        auto single = parseAndLayout(wrap(QStringLiteral(
            "<rect height=\"1\"/>")), QRectF());
        QVERIFY(single != nullptr);
        QCOMPARE(single->node->rect.size(), QSizeF(0, 1));

        // 全缺省：0×0
        auto none = parseAndLayout(wrap(QStringLiteral("<rect/>")), QRectF());
        QVERIFY(none != nullptr);
        QCOMPARE(none->node->rect.size(), QSizeF(0, 0));

        // 盒模型装饰计入：margin 2×2 + border 1×2 + padding 3×2 = 12
        auto deco = parseAndLayout(wrap(QStringLiteral(
            "<rect width=\"10\" height=\"10\" margin=\"2\" padding=\"3\" "
            "border-style=\"solid\" border-width=\"1\"/>")), QRectF());
        QVERIFY(deco != nullptr);
        QCOMPARE(deco->node->rect.size(), QSizeF(22, 22));
    }

    /// @brief 用例2：column 中 `<rect height="1">` 未给宽度 → 交叉轴恒填充至列宽；
    ///        默认 stretch 与 cross-align="start"（fillsCrossAxis 生效场景）行为一致。
    void columnFillsCrossAxis()
    {
        for (const char* align : {"stretch", "start"}) {
            const QString xml = wrap(QStringLiteral(
                "<column cross-align=\"%1\" space=\"0\">"
                "<text width=\"100\" height=\"10\">x</text>"
                "<rect height=\"1\" background-color=\"#ff0000\"/>"
                "</column>").arg(QString::fromLatin1(align)));
            auto result = parseAndLayout(xml, QRectF(0, 0, 200, 100));
            QVERIFY2(result != nullptr, align);
            const auto& children = result->node->children;
            QCOMPARE(children.size(), size_t(2));
            QVERIFY(asRect(children[1].get()) != nullptr);
            QCOMPARE(children[1]->rect.width(), 200.0);
            QCOMPARE(children[1]->rect.height(), 1.0);
        }
    }

    /// @brief 用例3：row 中对偶——`<rect width="1">` 未给高度 → 交叉轴恒填充至行高。
    void rowFillsCrossAxis()
    {
        for (const char* align : {"stretch", "center"}) {
            const QString xml = wrap(QStringLiteral(
                "<row cross-align=\"%1\" space=\"0\">"
                "<rect width=\"1\" background-color=\"#ff0000\"/>"
                "<text width=\"20\" height=\"10\">y</text>"
                "</row>").arg(QString::fromLatin1(align)));
            auto result = parseAndLayout(xml, QRectF(0, 0, 100, 50));
            QVERIFY2(result != nullptr, align);
            const auto& children = result->node->children;
            QCOMPARE(children.size(), size_t(2));
            QVERIFY(asRect(children[0].get()) != nullptr);
            QCOMPARE(children[0]->rect.width(), 1.0);
            QCOMPARE(children[0]->rect.height(), 50.0);
        }
    }

    /// @brief 用例4：显式尺寸豁免——`<rect width="40" height="2">` 在 column 中不被
    ///        拉伸，cross-align="center" 时居中（accent bar 场景）。
    void explicitSizeExemptAndCentered()
    {
        const QString xml = wrap(QStringLiteral(
            "<column cross-align=\"center\" space=\"0\">"
            "<text width=\"200\" height=\"10\">t</text>"
            "<rect width=\"40\" height=\"2\" background-color=\"#ff0000\"/>"
            "</column>"));
        auto result = parseAndLayout(xml, QRectF(0, 0, 200, 100));
        QVERIFY(result != nullptr);
        const auto& children = result->node->children;
        QCOMPARE(children.size(), size_t(2));
        QVERIFY(asRect(children[1].get()) != nullptr);
        QCOMPARE(children[1]->rect.size(), QSizeF(40, 2));
        QCOMPARE(children[1]->rect.x(), 80.0);  // (200 - 40) / 2 居中
    }

    /// @brief 用例5：绑定——b:width/b:height/b:background-color 正常求值；
    ///        rect 无布局策略属性（BI-P-023 不可达），以不在支持集合内的 b:space
    ///        验证拒绝路径（BI-P-012，不注册）作回归防御。
    void bindingEvaluationAndRejection()
    {
        LayoutResult r;
        r.root = XmlLayoutParser::parseString(wrap(QStringLiteral(
            "<rect b:width=\"w\" b:height=\"h\" b:background-color=\"bg\"/>")));
        QVERIFY(r.root != nullptr);
        r.propCtx.setProperty(QStringLiteral("w"), 40);
        r.propCtx.setProperty(QStringLiteral("h"), 2);
        r.propCtx.setProperty(QStringLiteral("bg"), QStringLiteral("#aabbcc"));

        LayoutContext lctx{&r.propCtx};
        r.node = LayoutEngine::materialize(r.root, lctx);
        QVERIFY(r.node != nullptr);
        QCOMPARE(r.node->style.width, 40.0);
        QCOMPARE(r.node->style.height, 2.0);
        QVERIFY(r.node->style.background.enabled);
        QCOMPARE(r.node->style.background.color, QColor(QStringLiteral("#aabbcc")));
        QVERIFY(r.root->bindsProperty(QStringLiteral("w")));
        QVERIFY(r.root->bindsProperty(QStringLiteral("bg")));

        // 拒绝路径回归防御：b:space 不在支持集合 → BI-P-012，绑定不注册
        CaptureCollector cap;
        auto bad = XmlLayoutParser::parseString(wrap(QStringLiteral(
            "<rect b:space=\"s\"/>")), &cap);
        QVERIFY(bad != nullptr);
        QVERIFY(cap.find(ErrorCode::UnknownBindingAttribute) != nullptr);
        QVERIFY(!bad->bindsProperty(QStringLiteral("s")));
    }

    /// @brief 用例6：退化——cell 内不填充（未指定维度保持 0）；rect 作根唯一子元素
    ///        全缺省测量/布局为 0×0，不崩溃。
    void degenerateInCellAndRoot()
    {
        // cell 不拉伸子部件：rect 未给宽度 → 宽保持 0，显式高度保留
        const QString xml = wrap(QStringLiteral(
            "<grid columns=\"1\" rows=\"1\">"
            "<cell><rect height=\"2\" background-color=\"#ff0000\"/></cell>"
            "</grid>"));
        auto result = parseAndLayout(xml, QRectF(0, 0, 100, 50));
        QVERIFY(result != nullptr);
        const auto& cells = result->node->children;
        QCOMPARE(cells.size(), size_t(1));
        QVERIFY(!cells[0]->children.empty());
        const Node* rectNode = cells[0]->children[0].get();
        QVERIFY(asRect(rectNode) != nullptr);
        QCOMPARE(rectNode->rect.width(), 0.0);
        QCOMPARE(rectNode->rect.height(), 2.0);

        // 根元素退化：全缺省 0×0，整条流水线不崩溃
        auto root = parseAndLayout(wrap(QStringLiteral("<rect/>")), QRectF());
        QVERIFY(root != nullptr);
        QCOMPARE(root->node->rect, QRectF(0, 0, 0, 0));
    }

    /// @brief 用例7：叶子含子元素解析失败（BI-P-007，Abort 级）。
    void leafWithChildrenFails()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(wrap(QStringLiteral(
            "<rect><text>x</text></rect>")), &cap);
        QVERIFY(root == nullptr);
        QVERIFY(cap.find(ErrorCode::LeafWithChildren) != nullptr);
    }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestRectElement)
#include "test_rect_element.moc"
