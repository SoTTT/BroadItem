#include <QtTest/QtTest>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/compat/QtCompat.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/context/QPropertyContext.h>
#include <broaditem/core/LayoutEngine.h>
#include <broaditem/parser/LayoutRegistry.h>
#include <broaditem/element/Element.h>
#include <broaditem/element/control/IfElement.h>
#include <broaditem/element/control/ForElement.h>

using namespace BroadItem;

/// @brief 供 IfElement 空值测试使用的最小可渲染元素（模板/实例分离版）。
class NullTestElement : public BroadItem::Element {
public:
    std::unique_ptr<BroadItem::Node> materialize(const BroadItem::LayoutContext&) const override
    {
        auto node = BroadItem::makeUnique<BroadItem::Node>();
        node->element = this;
        return node;
    }
    BroadItem::MeasureResult measure(const BroadItem::LayoutContext&,
                                     const BroadItem::LayoutConstraints&,
                                     BroadItem::Node&) const override
    {
        return {QSizeF(100, 20)};
    }
    void layout(const BroadItem::LayoutContext&, const QRectF&, BroadItem::Node&) const override {}
    void render(QPainter*, const BroadItem::LayoutContext&, const BroadItem::Node&) const override {}
    bool bindsProperty(const QString&) const override { return false; }
};

// 辅助类：声明了 Q_PROPERTY 的 QObject，用于测试 <if> 的空值行为
class IfNullHelper : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString warning READ warning WRITE setWarning NOTIFY warningChanged)
public:
    QString warning() const { return m_warning; }
    void setWarning(const QString& v)
    {
        if (m_warning != v) { m_warning = v; emit warningChanged(); }
    }
signals:
    void warningChanged();
private:
    QString m_warning; // 默认值 QString() 为 null
};

/// @brief 文件作用域消息捕获器：安装后收集所有 qWarning/qCritical 消息
static QStringList s_warnings;

static void captureWarning(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    Q_UNUSED(type)
    s_warnings.append(msg);
}

class TestParser : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void testParseSimpleText();
    void testParseColumnWithChildren();
    void testParseDecorators();
    void testMeasureText();
    void testColumnMeasure();
    void testBindProperty();
    void testIf();
    void testIfNullWithQPropertyContext();
    void testIfRequiresProp();
    void testIfHasTagRemoved();
    void testIfEqualsLiteral();
    void testIfEqualsConversions();
    void testIfBoundEquals();
    void testIfEqualsLiteralAndBoundMutuallyExclusive();
    void testIfNotBindingRejected();
    void testRegistryLoad();
    void testUnknownAttributeWarning();
    void testInvalidChildError();
    void testInvalidAttributeTypeError();
    void testAutoWrapFor();
    void testAutoWrapIf();
    void testAutoWrapBothForAndIf();
    void testNamespaceDeclarationSkipped();
    void testBindingAttributeSkipped();
    void testOldSyntaxRejected();
    void testNamespaceMissingRejected();
    void testWrongNamespaceUri();
};

/// @brief 解析最简单的文本元素并验证根节点不为空
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

/// @brief 解析带子元素的列布局
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

/// @brief 解析带装饰器属性（margin、border、background、padding）的元素
void TestParser::testParseDecorators()
{
    QString xml = R"(
        <root>
            <text margin="4" border-width="1" border-color="#555" border-radius="4"
                  background-color="#333" background-radius="3" padding="8">设备状态</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
}

/// @brief 测量文本元素，验证内容尺寸大于零
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
    auto node = BroadItem::LayoutEngine::materialize(root, ctx);
    QVERIFY(node != nullptr);
    auto result = node->element->measure(ctx, constraints, *node);
    QVERIFY(result.intrinsicSize.width() > 0);
    QVERIFY(result.intrinsicSize.height() > 0);
}

/// @brief 测量列布局，验证包含多个文本子元素时的尺寸计算
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
    auto node = BroadItem::LayoutEngine::materialize(root, ctx);
    QVERIFY(node != nullptr);
    auto result = node->element->measure(ctx, constraints, *node);
    QVERIFY(result.intrinsicSize.width() > 0);
    QVERIFY(result.intrinsicSize.height() > 0);
}

/// @brief 解析带数据绑定属性的元素，验证绑定关系正确注册
void TestParser::testBindProperty()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <text b:content="title">Default</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
    QVERIFY(root->bindsProperty("title"));
    QVERIFY(!root->bindsProperty("other"));
}

/// @brief 验证 <if> 条件控制元素（裸 b:prop 存在性语义）：属性不存在时不物化，存在时正常物化测量
void TestParser::testIf()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <if b:prop="show">
                <text>Visible</text>
            </if>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    BroadItem::LayoutContext ctx;
    BroadItem::MapPropertyContext mapCtx;
    ctx.ctx = &mapCtx;

    // 属性不存在 → 物化结果为空（新架构下"尺寸为零"的等价语义：
    // 控制元素透明，不产生任何实例节点）
    auto node = BroadItem::LayoutEngine::materialize(root, ctx);
    QVERIFY(node == nullptr);

    mapCtx.setProperty("show", "yes");
    node = BroadItem::LayoutEngine::materialize(root, ctx);
    QVERIFY(node != nullptr);
    // 根为控制元素（if），三阶段须经 node->element（物化后的可渲染模板）驱动，
    // 直接对控制元素模板调 measure 会命中 Element 基类的 qFatal
    auto result = node->element->measure(ctx, BroadItem::LayoutConstraints{}, *node);
    QVERIFY(result.intrinsicSize.width() > 0);
}

/// @brief 验证 QPropertyContext 中 null 值的 <if> 行为：默认 null 不展开，设非 null 后展开
void TestParser::testIfNullWithQPropertyContext()
{
    IfNullHelper item;
    BroadItem::QPropertyContext propCtx(&item);

    auto ifEl = std::make_shared<BroadItem::IfElement>();
    ifEl->setBindProperty("warning");
    ifEl->setChild(std::make_shared<NullTestElement>());

    BroadItem::LayoutContext ctx;
    ctx.ctx = &propCtx;

    // 默认 QString() 为 null → materializeChildren 返回空
    auto result = ifEl->materializeChildren(ctx);
    QCOMPARE(result.size(), size_t(0));

    // 设置为非 null 值 → materializeChildren 返回 1 个子节点
    item.setWarning("alert");
    result = ifEl->materializeChildren(ctx);
    QCOMPARE(result.size(), size_t(1));
}

/// @brief 验证 <if> 缺少 b:prop 时解析失败（条件必须有所作用的路径）
void TestParser::testIfRequiresProp()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <if>
                <text>Visible</text>
            </if>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root == nullptr);
}

/// @brief 验证 <if-has> 标签已移除：按未知标签处理，作为 root 唯一子元素时解析失败
void TestParser::testIfHasTagRemoved()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <if-has b:prop="show">
                <text>Visible</text>
            </if-has>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root == nullptr);
}

/// @brief 验证 equals 字面量比较：匹配/不匹配/缺失/null/not 取反/空串语义
void TestParser::testIfEqualsLiteral()
{
    auto makeIf = [](const QString& equals, bool withNot = false) {
        auto ifEl = std::make_shared<BroadItem::IfElement>();
        ifEl->setBindProperty("status");
        ifEl->setEquals(equals);
        ifEl->setNot(withNot);
        ifEl->setChild(std::make_shared<NullTestElement>());
        return ifEl;
    };

    BroadItem::MapPropertyContext mapCtx;
    BroadItem::LayoutContext ctx;
    ctx.ctx = &mapCtx;

    // 匹配 → 1 个节点；不匹配 → 0 个节点
    mapCtx.setProperty("status", "alarm");
    QCOMPARE(makeIf("alarm")->materializeChildren(ctx).size(), size_t(1));
    QCOMPARE(makeIf("ok")->materializeChildren(ctx).size(), size_t(0));

    // not：值不等 → 显示；值相等 → 隐藏
    QCOMPARE(makeIf("ok", true)->materializeChildren(ctx).size(), size_t(1));
    QCOMPARE(makeIf("alarm", true)->materializeChildren(ctx).size(), size_t(0));

    // 属性缺失 → 不显示；not → 显示（整体取反）
    BroadItem::MapPropertyContext emptyCtx;
    BroadItem::LayoutContext ctx2;
    ctx2.ctx = &emptyCtx;
    QCOMPARE(makeIf("alarm")->materializeChildren(ctx2).size(), size_t(0));
    QCOMPARE(makeIf("alarm", true)->materializeChildren(ctx2).size(), size_t(1));

    // null 值 → 不显示
    mapCtx.setProperty("status", QVariant(QString()));
    QCOMPARE(makeIf("alarm")->materializeChildren(ctx).size(), size_t(0));

    // equals="" 匹配非 null 空串，不匹配 null
    mapCtx.setProperty("status", QString(""));
    QCOMPARE(makeIf("")->materializeChildren(ctx).size(), size_t(1));
    mapCtx.setProperty("status", QVariant(QString()));
    QCOMPARE(makeIf("")->materializeChildren(ctx).size(), size_t(0));
}

/// @brief 钉死 equals 字符串化比较的 QVariant 隐式转换行为（Qt5 toString 语义）
void TestParser::testIfEqualsConversions()
{
    auto ifEl = std::make_shared<BroadItem::IfElement>();
    ifEl->setBindProperty("v");
    ifEl->setChild(std::make_shared<NullTestElement>());

    BroadItem::MapPropertyContext mapCtx;
    BroadItem::LayoutContext ctx;
    ctx.ctx = &mapCtx;

    auto matches = [&](const QVariant& value, const QString& equals) {
        mapCtx.setProperty("v", value);
        ifEl->setEquals(equals);
        return ifEl->materializeChildren(ctx).size() == size_t(1);
    };

    // int → "3"、"-2"、"0"
    QVERIFY(matches(3, "3"));
    QVERIFY(!matches(3, "03"));
    QVERIFY(matches(-2, "-2"));
    QVERIFY(matches(0, "0"));

    // double：Qt5 toString 尾随零丢失（3.0 → "3"）
    QVERIFY(matches(3.0, "3"));
    QVERIFY(!matches(3.0, "3.0"));
    QVERIFY(matches(3.5, "3.5"));
    QVERIFY(matches(-0.5, "-0.5"));

    // bool → "true"/"false"
    QVERIFY(matches(true, "true"));
    QVERIFY(!matches(true, "1"));
    QVERIFY(matches(false, "false"));

    // QString：区分大小写、区分首尾空白
    QVERIFY(matches(QString("alarm"), "alarm"));
    QVERIFY(!matches(QString("alarm"), "Alarm"));
    QVERIFY(!matches(QString("alarm"), "alarm "));
}

/// @brief 验证 b:equals 绑定比较值：匹配/不匹配/不可解析/数字目标/绑定发现
void TestParser::testIfBoundEquals()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <if b:prop="status" b:equals="level">
                <text>ALARM</text>
            </if>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
    auto ifEl = std::dynamic_pointer_cast<BroadItem::IfElement>(root);
    QVERIFY(ifEl != nullptr);

    // b:prop 路径与 b:equals 目标均参与绑定发现（变更触发重布局）
    QVERIFY(ifEl->bindsProperty("status"));
    QVERIFY(ifEl->bindsProperty("level"));
    QVERIFY(!ifEl->bindsProperty("other"));

    BroadItem::MapPropertyContext mapCtx;
    BroadItem::LayoutContext ctx;
    ctx.ctx = &mapCtx;
    mapCtx.setProperty("status", "alarm");

    // 绑定目标匹配 → 显示；不匹配 → 隐藏（运行时可改比较目标）
    mapCtx.setProperty("level", "alarm");
    QCOMPARE(ifEl->materializeChildren(ctx).size(), size_t(1));
    mapCtx.setProperty("level", "ok");
    QCOMPARE(ifEl->materializeChildren(ctx).size(), size_t(0));

    // 绑定路径不可解析 → 条件不成立
    BroadItem::MapPropertyContext noLevel;
    noLevel.setProperty("status", "alarm");
    BroadItem::LayoutContext ctx2;
    ctx2.ctx = &noLevel;
    QCOMPARE(ifEl->materializeChildren(ctx2).size(), size_t(0));

    // 绑定路径不可解析 + not → 整体取反，显示
    {
        QString notXml = R"(
            <root xmlns:b="urn:broaditem:binding">
                <if b:prop="status" b:equals="level" not="true">
                    <text>ALARM</text>
                </if>
            </root>
        )";
        auto notRoot = BroadItem::XmlLayoutParser::parseString(notXml);
        QVERIFY(notRoot != nullptr);
        auto notIfEl = std::dynamic_pointer_cast<BroadItem::IfElement>(notRoot);
        QVERIFY(notIfEl != nullptr);
        QCOMPARE(notIfEl->materializeChildren(ctx2).size(), size_t(1));
    }

    // 绑定目标为数字时同样走字符串化比较（两侧均 toString）
    mapCtx.setProperty("level", 3);
    mapCtx.setProperty("status", "3");
    QCOMPARE(ifEl->materializeChildren(ctx).size(), size_t(1));
}

/// @brief 验证 equals 字面量与 b:equals 同现时互斥：qCritical，绑定被忽略，按字面量求值
void TestParser::testIfEqualsLiteralAndBoundMutuallyExclusive()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <if b:prop="status" equals="alarm" b:equals="level">
                <text>ALARM</text>
            </if>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
    auto ifEl = std::dynamic_pointer_cast<BroadItem::IfElement>(root);
    QVERIFY(ifEl != nullptr);

    // 互斥：b:equals 绑定未注册
    QVERIFY(!ifEl->bindsProperty("level"));

    // 按字面量 equals="alarm" 求值，与 level 无关
    BroadItem::MapPropertyContext mapCtx;
    BroadItem::LayoutContext ctx;
    ctx.ctx = &mapCtx;
    mapCtx.setProperty("status", "alarm");
    mapCtx.setProperty("level", "different");
    QCOMPARE(ifEl->materializeChildren(ctx).size(), size_t(1));
}

/// @brief 验证 b:not 被拒绝：qWarning 且按无 not 求值（not 是结构性修饰符，不参与绑定）
void TestParser::testIfNotBindingRejected()
{
    s_warnings.clear();
    QtMessageHandler original = qInstallMessageHandler(captureWarning);

    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <if b:prop="show" b:not="flag">
                <text>Visible</text>
            </if>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    qInstallMessageHandler(original);
    QVERIFY(root != nullptr);

    bool warned = false;
    for (const QString& w : s_warnings) {
        if (w.contains("b:not"))
            warned = true;
    }
    QVERIFY2(warned, "b:not should be rejected with a warning");

    // b:not 被忽略：按无 not 求值（属性缺失 → 不显示；存在 → 显示）
    auto ifEl = std::dynamic_pointer_cast<BroadItem::IfElement>(root);
    QVERIFY(ifEl != nullptr);
    BroadItem::MapPropertyContext mapCtx;
    BroadItem::LayoutContext ctx;
    ctx.ctx = &mapCtx;
    QCOMPARE(ifEl->materializeChildren(ctx).size(), size_t(0));
    mapCtx.setProperty("show", "yes");
    QCOMPARE(ifEl->materializeChildren(ctx).size(), size_t(1));
}

/// @brief 验证从目录加载布局注册表，至少加载到 test_layout.xml
void TestParser::testRegistryLoad()
{
    int count = BroadItem::loadLayoutsFromDirectory(".");
    // 应能加载到 test_layout.xml
    QVERIFY(count >= 1);
}

/// @brief 验证未知属性触发 qWarning 但不中断解析，仍能成功构建元素
void TestParser::testUnknownAttributeWarning()
{
    // 未知属性应触发 qWarning 但不导致解析失败
    QString xml = R"(
        <root>
            <text unknown-attr="value" font-size="12">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
}

/// @brief 验证文本元素包含子元素时解析失败
void TestParser::testInvalidChildError()
{
    // 文本元素不应有子元素 → 解析应失败
    QString xml = R"(
        <root>
            <text font-size="12">
                <text>Nested</text>
            </text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root == nullptr);
}

/// @brief 验证非法属性类型触发 qCritical 但使用默认值继续解析
void TestParser::testInvalidAttributeTypeError()
{
    // 非法类型应触发 qCritical，但解析仍应以默认值成功继续
    QString xml = R"(
        <root>
            <text font-size="not-a-number">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);
}
/// @brief 验证 @c b:of 和 @c b:as 的自动包装：非控制元素带绑定属性时自动创建 ForElement 包装器
void TestParser::testAutoWrapFor()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <row b:of="list" b:as="item" space="4">
                <text b:content="item.name"/>
            </row>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    auto forEl = std::dynamic_pointer_cast<BroadItem::ForElement>(root);
    QVERIFY(forEl != nullptr);
    QVERIFY(forEl->bindsProperty("list"));

    BroadItem::MapPropertyContext mapCtx;
    mapCtx.setProperty("list", QStringList{"a", "b"});

    BroadItem::LayoutContext ctx;
    ctx.ctx = &mapCtx;

    auto expanded = forEl->materializeChildren(ctx);
    QCOMPARE(expanded.size(), size_t(2));
}

/// @brief 验证 @c b:prop 的自动包装：非控制元素带 @c b:prop 属性时自动创建 IfElement 包装器
void TestParser::testAutoWrapIf()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <text b:prop="show">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    auto ifEl = std::dynamic_pointer_cast<BroadItem::IfElement>(root);
    QVERIFY(ifEl != nullptr);
    QVERIFY(ifEl->bindsProperty("show"));

    BroadItem::MapPropertyContext mapCtx;
    BroadItem::LayoutContext ctx;
    ctx.ctx = &mapCtx;

    // 属性不存在 → materializeChildren 返回 0 个节点
    auto result = ifEl->materializeChildren(ctx);
    QCOMPARE(result.size(), size_t(0));

    // 属性存在 → materializeChildren 返回 1 个实例节点
    mapCtx.setProperty("show", "yes");
    result = ifEl->materializeChildren(ctx);
    QCOMPARE(result.size(), size_t(1));
}

/// @brief 验证 @c b:of 和 @c b:prop 同时存在时，wrapFor 优先于 wrapIf
void TestParser::testAutoWrapBothForAndIf()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <text b:of="list" b:as="x" b:prop="show" b:content="x.name"/>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    // wrapFor 优先于 wrapIf → 应得到 ForElement
    auto forEl = std::dynamic_pointer_cast<BroadItem::ForElement>(root);
    QVERIFY(forEl != nullptr);
}
/// @brief 验证 @c xmlns:b 命名空间声明不触发 "unknown attribute" 警告
void TestParser::testNamespaceDeclarationSkipped()
{
    s_warnings.clear();
    QtMessageHandler original = qInstallMessageHandler(captureWarning);

    QString xml = R"(
        <root>
            <text xmlns:b="urn:broaditem:binding" font-size="12">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    qInstallMessageHandler(original);

    QVERIFY(root != nullptr);

    for (const QString& w : s_warnings) {
        QVERIFY2(!w.contains("unknown attribute", Qt::CaseInsensitive),
                 qPrintable(QString("Unexpected 'unknown attribute' warning: %1").arg(w)));
    }
}

/// @brief 验证 @c b:content / @c b:of / @c b:as / @c b:prop 绑定属性不触发 "unknown attribute" 警告
void TestParser::testBindingAttributeSkipped()
{
    s_warnings.clear();
    QtMessageHandler original = qInstallMessageHandler(captureWarning);

    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <text b:content="title" font-size="12">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    qInstallMessageHandler(original);

    QVERIFY(root != nullptr);

    for (const QString& w : s_warnings) {
        QVERIFY2(!w.contains("unknown attribute", Qt::CaseInsensitive),
                 qPrintable(QString("Unexpected 'unknown attribute' warning: %1").arg(w)));
    }
}

/// @brief 验证旧 @c :xxx 语法不被识别为绑定属性。
/// @details Qt5：namespace processing 下 @c :content 按普通属性名保留，parse 成功但无绑定；
/// Qt6：XML 解析器直接拒绝裸冒号属性名（BI-P-002 语法错误，parse 失败）。
/// 两版语义等价——旧语法都不会产生绑定，差异仅在拒绝发生的阶段。
void TestParser::testOldSyntaxRejected()
{
    QString xml = R"(
        <root>
            <text :content="title">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6 在语法层拒绝：parse 返回 nullptr
    QVERIFY(root == nullptr);
#else
    QVERIFY(root != nullptr);

    // 旧 :content 语法在 namespace processing 下不被识别为绑定属性，
    // TextElement 不应绑定 title
    QVERIFY2(!root->bindsProperty("title"),
             "旧 :content 语法不应被识别为绑定属性");
    QVERIFY2(!root->bindsProperty("other"),
             "旧 :content 语法不应被识别为绑定属性");
#endif
}

/// @brief 验证缺少命名空间声明时 @c b:content 不被识别为绑定属性。
/// @details Qt5：未声明 @c xmlns:b 时前缀未绑定，@c hasAttributeNS 返回 false，
/// parse 成功但无绑定；Qt6：未声明前缀是致命语法错误（BI-P-002），parse 失败。
/// 两版语义等价——未声明的 b: 属性都不会产生绑定。
void TestParser::testNamespaceMissingRejected()
{
    QString xml = R"(
        <root>
            <text b:content="title">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QVERIFY(root == nullptr);
#else
    QVERIFY(root != nullptr);

    // 未声明 xmlns:b 时，b:content 的 b: 前缀未绑定到 BINDING_NS 命名空间 URI，
    // hasAttributeNS(BINDING_NS, "content") 返回 false，TextElement 无绑定
    QVERIFY2(!root->bindsProperty("title"),
             "未声明 xmlns:b 时 b:content 不应被识别为绑定");
    QVERIFY2(!root->bindsProperty("other"),
             "未声明 xmlns:b 时 b:content 不应被识别为绑定");
#endif
}

/// @brief 验证错误命名空间 URI 时绑定属性不被识别。
/// @details 声明 @c xmlns:b="urn:wrong:uri" 时，@c hasAttributeNS(BINDING_NS, "content")
/// 返回 false（BINDING_NS 是 "urn:broaditem:binding"），因此 TextElement 不会设置绑定。
void TestParser::testWrongNamespaceUri()
{
    QString xml = R"(
        <root xmlns:b="urn:wrong:uri">
            <text b:content="title">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    // 根元素不应被 auto-wrap 为 ForElement 或 IfElement
    QVERIFY2(std::dynamic_pointer_cast<BroadItem::ForElement>(root) == nullptr,
             "错误命名空间 URI 时不应被自动包装为 ForElement");
    QVERIFY2(std::dynamic_pointer_cast<BroadItem::IfElement>(root) == nullptr,
             "错误命名空间 URI 时不应被自动包装为 IfElement");

    // hasAttributeNS(BINDING_NS, ...) 不匹配，因此不应绑定任何属性
    QVERIFY2(!root->bindsProperty("title"),
             "错误命名空间 URI 时 b:content 不应被识别为绑定");
    QVERIFY2(!root->bindsProperty("other"),
             "错误命名空间 URI 时 b:content 不应被识别为绑定");
}

// NOLINTEND(readability-convert-member-functions-to-static)

QTEST_MAIN(TestParser)
#include "test_parser.moc"
