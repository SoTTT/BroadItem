#include <QtTest/QtTest>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/element/Element.h>
#include <broaditem/element/text/TextElement.h>
#include <broaditem/element/control/ForElement.h>
#include <broaditem/core/Node.h>
#include <broaditem/core/Frame.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/expression/Binding.h>

#include "helpers/binding_helpers.h"
#include "helpers/frame_helpers.h"

using namespace BroadItem;
using namespace BroadItem::TestHelpers;

class TestBoundAttributes : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    /// @brief 用例1：b:color 注册进通用绑定表，bindingFor 命中且路径正确。
    void parseRegistersBinding()
    {
        auto root = parse(QStringLiteral("<text b:color=\"c\">x</text>"));
        QVERIFY2(root, "解析失败");
        const Binding* b = root->bindingFor(QStringLiteral("color"));
        QVERIFY2(b, "b:color 未注册进通用绑定表");
        QCOMPARE(b->path(), QStringLiteral("c"));
    }

    /// @brief 用例2：字面量 color 与 b:color 互斥，绑定被忽略（qCritical 路径）。
    void mutualExclusionIgnoresBinding()
    {
        auto root = parse(QStringLiteral("<text color=\"red\" b:color=\"c\">x</text>"));
        QVERIFY2(root, "解析失败");
        QVERIFY2(!root->bindingFor(QStringLiteral("color")),
                 "字面量与绑定同现时应忽略绑定");
    }

    /// @brief 用例3：未注册属性 b:colr 跳过；保留名 of/as 豁免（row 被包装为 for 后取模板验证）。
    void unknownAndReservedSkipped()
    {
        // 未注册属性：colr 不在 TextElement::supportedAttributes() 中
        auto text = parse(QStringLiteral("<text b:colr=\"c\">x</text>"));
        QVERIFY2(text, "解析失败");
        QVERIFY2(!text->bindingFor(QStringLiteral("colr")),
                 "未注册属性 b:colr 不应进入绑定表");
        QVERIFY2(!text->bindingFor(QStringLiteral("color")),
                 "b:colr 不应误注册为 color");

        // 保留名豁免：row 携带 b:of/b:as 时被解析器包装为 ForElement，
        // 内层 row 的通用绑定表不得收录保留名 of/as。
        auto wrapped = parse(QStringLiteral("<row b:of=\"items\" b:as=\"i\"/>"));
        QVERIFY2(wrapped, "解析失败");
        auto forEl = std::dynamic_pointer_cast<ForElement>(wrapped);
        QVERIFY2(forEl, "携带 b:of 的 row 应被包装为 ForElement");
        auto row = forEl->templateElement();
        QVERIFY2(row, "ForElement 模板为空");
        QVERIFY2(!row->bindingFor(QStringLiteral("of")),
                 "保留名 of 应豁免，不进入通用绑定表");
        QVERIFY2(!row->bindingFor(QStringLiteral("as")),
                 "保留名 as 应豁免，不进入通用绑定表");
    }

    /// @brief 用例4：上下文命中时物化求值颜色绑定，TextNode.color 解析为绑定值。
    void resolveColorHit()
    {
        auto root = parse(QStringLiteral("<text b:color=\"c\">x</text>"));
        QVERIFY2(root, "解析失败");
        MapPropertyContext map;
        map.setProperty(QStringLiteral("c"), QStringLiteral("#FF0000"));
        auto node = materializeFirst(root, map);
        QVERIFY2(node, "物化失败");
        const auto& textNode = static_cast<const TextNode&>(*node);
        QCOMPARE(textNode.color, QColor(QStringLiteral("#FF0000")));
    }

    /// @brief 用例5：属性缺失时静默回退，TextNode.color 保持默认黑。
    void resolveColorMissingFallsBack()
    {
        auto root = parse(QStringLiteral("<text b:color=\"c\">x</text>"));
        QVERIFY2(root, "解析失败");
        MapPropertyContext map;  // 不注入 c
        auto node = materializeFirst(root, map);
        QVERIFY2(node, "物化失败");
        const auto& textNode = static_cast<const TextNode&>(*node);
        QCOMPARE(textNode.color, QColor(Qt::black));
    }

    /// @brief 用例6：绑定值类型失配（int 而非颜色字符串）时回退默认黑。
    void resolveColorTypeMismatchFallsBack()
    {
        auto root = parse(QStringLiteral("<text b:color=\"c\">x</text>"));
        QVERIFY2(root, "解析失败");
        MapPropertyContext map;
        map.setProperty(QStringLiteral("c"), 123);  // int，非颜色字符串
        auto node = materializeFirst(root, map);
        QVERIFY2(node, "物化失败");
        const auto& textNode = static_cast<const TextNode&>(*node);
        QCOMPARE(textNode.color, QColor(Qt::black));
    }

    /// @brief 用例7：盒模型绑定求值——b:padding 四边同值、b:background-color 启用背景。
    void resolveBoxModel()
    {
        auto root = parse(QStringLiteral("<text b:padding=\"p\" b:background-color=\"bg\">x</text>"));
        QVERIFY2(root, "解析失败");
        MapPropertyContext map;
        map.setProperty(QStringLiteral("p"), 8);
        map.setProperty(QStringLiteral("bg"), QStringLiteral("#00FF00"));
        auto node = materializeFirst(root, map);
        QVERIFY2(node, "物化失败");
        QCOMPARE(node->style.padding.top, 8.0);
        QCOMPARE(node->style.padding.bottom, 8.0);
        QCOMPARE(node->style.padding.left, 8.0);
        QCOMPARE(node->style.padding.right, 8.0);
        QCOMPARE(node->style.background.color, QColor(QStringLiteral("#00FF00")));
        QVERIFY2(node->style.background.enabled,
                 "background-color 绑定命中时应启用背景");
    }

    /// @brief 用例8：b:width 求值——命中时写入 style.width；类型失配回退空 Optional。
    void resolveWidthHeight()
    {
        auto root = parse(QStringLiteral("<text b:width=\"w\">x</text>"));
        QVERIFY2(root, "解析失败");

        MapPropertyContext map;
        map.setProperty(QStringLiteral("w"), 120);
        auto node = materializeFirst(root, map);
        QVERIFY2(node, "物化失败");
        QCOMPARE(node->style.width.value_or(0), 120.0);

        // 失配："abc" 无法转为 double，回退空（未指定）
        MapPropertyContext bad;
        bad.setProperty(QStringLiteral("w"), QStringLiteral("abc"));
        auto badNode = materializeFirst(root, bad);
        QVERIFY2(badNode, "物化失败");
        QVERIFY2(!badNode->style.width.has_value(), "类型失配时 style.width 应为空");

        // 负值：非法尺寸，视为未指定（与 parse 侧负值过滤对齐）
        MapPropertyContext neg;
        neg.setProperty(QStringLiteral("w"), -5);
        auto negNode = materializeFirst(root, neg);
        QVERIFY2(negNode, "物化失败");
        QVERIFY2(!negNode->style.width.has_value(), "负值绑定时 style.width 应为空");
    }

    /// @brief 用例9：bindsProperty 沿容器传播——column 聚合子文本的绑定。
    void bindsPropertyAggregation()
    {
        auto root = parse(QStringLiteral("<column><text b:color=\"alarmColor\"/></column>"));
        QVERIFY2(root, "解析失败");
        QVERIFY2(root->bindsProperty(QStringLiteral("alarmColor")),
                 "column 应聚合子元素的绑定（传播链）");
        QVERIFY2(!root->bindsProperty(QStringLiteral("other")),
                 "未绑定的属性名不应命中");
    }

    /// @brief 用例10：括号路径前缀匹配——绑定 items[0].color 命中属性名 items。
    void prefixMatchBracket()
    {
        auto root = parse(QStringLiteral("<text b:color=\"items[0].color\">x</text>"));
        QVERIFY2(root, "解析失败");
        QVERIFY2(root->bindsProperty(QStringLiteral("items")),
                 "items[0].color 应以前缀匹配 items");
        QVERIFY2(!root->bindsProperty(QStringLiteral("item")),
                 "item 不是 items[0].color 的前缀，不应命中");
    }

    /// @brief 用例11：刷新链像素级验证——setDynamicProperty 触发重物化+重渲染，
    /// 背景从透明（未启用）变为绑定绿色。采样点 (5,5) 位于 padding=10 的背景区，
    /// 避开文字字形与图像边缘。
    void refreshChainPixel()
    {
        auto frame = frameFromXmlString(
            QStringLiteral("<text b:background-color=\"bg\" padding=\"10\">Hi</text>"));
        QVERIFY2(frame, "Frame 构造失败");

        // 初始：bg 未注入 → 背景不启用 → (5,5) 为透明
        const QImage before = frame->toImage();
        QVERIFY2(!before.isNull(), "初始渲染为空");
        QVERIFY2(before.width() > 5 && before.height() > 5,
                 "初始图像尺寸不足，采样点越界");
        QCOMPARE(before.pixelColor(5, 5).alpha(), 0);

        // 注入 bg → 标脏并冲刷合并更新 → 背景启用并填绿
        frame->setDynamicProperty(QStringLiteral("bg"), QStringLiteral("#00FF00"));
        frame->flush();  // 默认 Coalesced 策略下立即执行待定重布局
        const QImage after = frame->toImage();
        QVERIFY2(!after.isNull(), "刷新后渲染为空");
        QVERIFY2(after.width() > 5 && after.height() > 5,
                 "刷新后图像尺寸不足，采样点越界");
        QCOMPARE(after.pixelColor(5, 5), QColor(0, 255, 0));
    }

    /// @brief 用例12（附加）：for 项级上下文解析——每次迭代的 b:color 绑定
    /// 经 ItemPropertyContext 解析到各自的 u.color。
    void itemContextResolution()
    {
        auto root = parse(QStringLiteral(
            "<column><for b:of=\"users\" b:as=\"u\">"
            "<text b:color=\"u.color\"/>"
            "</for></column>"));
        QVERIFY2(root, "解析失败");

        QVariantList users{
            QVariantMap{{QStringLiteral("name"), QStringLiteral("甲")},
                        {QStringLiteral("color"), QStringLiteral("#FF0000")}},
            QVariantMap{{QStringLiteral("name"), QStringLiteral("乙")},
                        {QStringLiteral("color"), QStringLiteral("#0000FF")}},
        };
        MapPropertyContext map;
        map.setProperty(QStringLiteral("users"), users);

        auto colNode = materializeFirst(root, map);
        QVERIFY2(colNode, "物化失败");
        QCOMPARE(colNode->children.size(), size_t(2));
        const auto& first = static_cast<const TextNode&>(*colNode->children[0]);
        const auto& second = static_cast<const TextNode&>(*colNode->children[1]);
        QCOMPARE(first.color, QColor(QStringLiteral("#FF0000")));
        QCOMPARE(second.color, QColor(QStringLiteral("#0000FF")));
    }
    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestBoundAttributes)
#include "test_bound_attributes.moc"
