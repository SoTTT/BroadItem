#include <QtTest/QtTest>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/context/QPropertyContext.h>
#include <broaditem/core/LayoutEngine.h>
#include <broaditem/parser/LayoutRegistry.h>
#include <broaditem/element/Element.h>
#include <broaditem/control/IfHasElement.h>
#include <broaditem/control/ForElement.h>
#include <QDebug>

// Minimal element for IfHasElement null value test
class NullTestElement : public BroadItem::Element {
public:
    BroadItem::MeasureResult measure(const BroadItem::LayoutContext&,
                                     const BroadItem::LayoutConstraints&) override
    {
        return {QSizeF(100, 20)};
    }
    void layout(const BroadItem::LayoutContext&, const QRectF& rect) override { m_rect = rect; }
    void render(QPainter*, const BroadItem::LayoutContext&) const override {}
    BroadItem::ElementPtr clone() const override { return std::make_shared<NullTestElement>(); }
    bool bindsProperty(const QString&) const override { return false; }
};

// Helper: QObject with declared Q_PROPERTY for testing <if-has> null value behavior
class IfHasNullHelper : public QObject {
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
    QString m_warning; // default: QString() is null
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
    void testIfHas();
    void testIfHasNullWithQPropertyContext();
    void testRegistryLoad();
    void testUnknownAttributeWarning();
    void testInvalidChildError();
    void testInvalidAttributeTypeError();
    void testAutoWrapFor();
    void testAutoWrapIfHas();
    void testAutoWrapBothForAndIfHas();
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
    auto result = root->measure(ctx, constraints);
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
    auto result = root->measure(ctx, constraints);
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

/// @brief 验证 <if-has> 条件控制元素：属性不存在时隐藏（尺寸为零），存在时显示
void TestParser::testIfHas()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <if-has b:prop="show">
                <text>Visible</text>
            </if-has>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    BroadItem::LayoutContext ctx;
    BroadItem::MapPropertyContext mapCtx;
    ctx.ctx = &mapCtx;

    // Property not set -> should not show
    auto result = root->measure(ctx, BroadItem::LayoutConstraints{});
    QCOMPARE(result.intrinsicSize.width(), 0.0);
    QCOMPARE(result.intrinsicSize.height(), 0.0);

    mapCtx.setProperty("show", "yes");
    result = root->measure(ctx, BroadItem::LayoutConstraints{});
    QVERIFY(result.intrinsicSize.width() > 0);
}

/// @brief 验证 QPropertyContext 中 null 值的 <if-has> 行为：默认 null 不展开，设非 null 后展开
void TestParser::testIfHasNullWithQPropertyContext()
{
    IfHasNullHelper item;
    BroadItem::QPropertyContext propCtx(&item);

    auto ifEl = std::make_shared<BroadItem::IfHasElement>();
    ifEl->setBindProperty("warning");
    ifEl->setChild(std::make_shared<NullTestElement>());

    BroadItem::LayoutContext ctx;
    ctx.ctx = &propCtx;

    // Default QString() is null -> expand returns empty
    auto result = ifEl->expand(ctx);
    QCOMPARE(result.size(), 0);

    // Set to non-null value -> expand returns cloned child
    item.setWarning("alert");
    result = ifEl->expand(ctx);
    QCOMPARE(result.size(), 1);
}

/// @brief 验证从目录加载布局注册表，至少加载到 test_layout.xml
void TestParser::testRegistryLoad()
{
    int count = BroadItem::loadLayoutsFromDirectory(".");
    // Should load test_layout.xml
    QVERIFY(count >= 1);
}

/// @brief 验证未知属性触发 qWarning 但不中断解析，仍能成功构建元素
void TestParser::testUnknownAttributeWarning()
{
    // Unknown attributes should trigger qWarning but not fail parsing
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
    // Text element should not have child elements -> parse should fail
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
    // Invalid type should trigger qCritical but parse should still succeed with default
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

    auto expanded = forEl->expand(ctx);
    QCOMPARE(expanded.size(), size_t(2));
}

/// @brief 验证 @c b:prop 的自动包装：非控制元素带 @c b:prop 属性时自动创建 IfHasElement 包装器
void TestParser::testAutoWrapIfHas()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <text b:prop="show">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    auto ifEl = std::dynamic_pointer_cast<BroadItem::IfHasElement>(root);
    QVERIFY(ifEl != nullptr);
    QVERIFY(ifEl->bindsProperty("show"));

    BroadItem::MapPropertyContext mapCtx;
    BroadItem::LayoutContext ctx;
    ctx.ctx = &mapCtx;

    // 属性不存在 → expand 返回 0 个子元素
    auto result = ifEl->expand(ctx);
    QCOMPARE(result.size(), size_t(0));

    // 属性存在 → expand 返回 1 个克隆子元素
    mapCtx.setProperty("show", "yes");
    result = ifEl->expand(ctx);
    QCOMPARE(result.size(), size_t(1));
}

/// @brief 验证 @c b:of 和 @c b:prop 同时存在时，wrapFor 优先于 wrapIfHas
void TestParser::testAutoWrapBothForAndIfHas()
{
    QString xml = R"(
        <root xmlns:b="urn:broaditem:binding">
            <text b:of="list" b:as="x" b:prop="show" b:content="x.name"/>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    // wrapFor 优先于 wrapIfHas → 应得到 ForElement
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
/// @details namespace processing 下 @c :content 没有命名空间前缀声明，
/// Qt 将其视为普通属性名 @c ":content"，@c hasAttributeNS(BINDING_NS, "content") 返回 false，
/// 因此 TextElement 不会设置绑定。
void TestParser::testOldSyntaxRejected()
{
    QString xml = R"(
        <root>
            <text :content="title">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    /// 旧 :content 语法在 namespace processing 下不被识别为绑定属性，
    /// TextElement 不应绑定 title
    QVERIFY2(!root->bindsProperty("title"),
             "Old :content syntax should not be recognized as binding attribute");
    QVERIFY2(!root->bindsProperty("other"),
             "Old :content syntax should not be recognized as binding attribute");
}

/// @brief 验证缺少命名空间声明时 @c b:content 不被识别为绑定属性。
/// @details 不声明 @c xmlns:b 时，@c b: 前缀未绑定到 BINDING_NS，
/// @c hasAttributeNS(BINDING_NS, "content") 返回 false，TextElement 不会设置绑定。
void TestParser::testNamespaceMissingRejected()
{
    QString xml = R"(
        <root>
            <text b:content="title">Hello</text>
        </root>
    )";
    auto root = BroadItem::XmlLayoutParser::parseString(xml);
    QVERIFY(root != nullptr);

    /// 未声明 xmlns:b 时，b:content 的 b: 前缀未绑定到 BINDING_NS 命名空间 URI，
    /// hasAttributeNS(BINDING_NS, "content") 返回 false，TextElement 无绑定
    QVERIFY2(!root->bindsProperty("title"),
             "b:content without xmlns:b declaration should not be recognized as binding");
    QVERIFY2(!root->bindsProperty("other"),
             "b:content without xmlns:b declaration should not be recognized as binding");
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

    /// 根元素不应被 auto-wrap 为 ForElement 或 IfHasElement
    QVERIFY2(std::dynamic_pointer_cast<BroadItem::ForElement>(root) == nullptr,
             "Should not auto-wrap to ForElement with wrong namespace URI");
    QVERIFY2(std::dynamic_pointer_cast<BroadItem::IfHasElement>(root) == nullptr,
             "Should not auto-wrap to IfHasElement with wrong namespace URI");

    /// hasAttributeNS(BINDING_NS, ...) 不匹配，因此不应绑定任何属性
    QVERIFY2(!root->bindsProperty("title"),
             "b:content with wrong namespace URI should not be recognized as binding");
    QVERIFY2(!root->bindsProperty("other"),
             "b:content with wrong namespace URI should not be recognized as binding");
}

// NOLINTEND(readability-convert-member-functions-to-static)

QTEST_MAIN(TestParser)
#include "test_parser.moc"
