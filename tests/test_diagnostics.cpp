#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/parser/LayoutRegistry.h>
#include <broaditem/core/Frame.h>
#include <broaditem/core/Node.h>
#include <broaditem/element/text/TextElement.h>
#include <broaditem/element/layout/GridLayout.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/context/QPropertyContext.h>

using namespace BroadItem;

/// @brief 捕获型收集器：记录全部诊断供断言。
class CaptureCollector : public ErrorCollector {
public:
    void report(const Diagnostic& d) override { list.append(d); }

    QList<Diagnostic> list;  ///< 已捕获的诊断序列。

    /// @brief 统计指定错误码出现次数。
    int count(ErrorCode c) const
    {
        int n = 0;
        for (const auto& d : list)
            if (d.code == c)
                ++n;
        return n;
    }

    /// @brief 查找指定错误码的首条诊断（未命中返回 nullptr）。
    const Diagnostic* find(ErrorCode c) const
    {
        for (const auto& d : list)
            if (d.code == c)
                return &d;
        return nullptr;
    }
};

/// @brief 进程级收集器守卫：构造注入、析构恢复默认。
struct ProcessCollectorGuard {
    explicit ProcessCollectorGuard(const std::shared_ptr<ErrorCollector>& c)
    {
        Diagnostics::setCollector(c);
    }
    ~ProcessCollectorGuard() { Diagnostics::setCollector(nullptr); }
};

/// @brief 包裹 root 与绑定命名空间声明。
static QString wrap(const QString& inner)
{
    return QStringLiteral("<root xmlns:b=\"urn:broaditem:binding\">") + inner
           + QStringLiteral("</root>");
}

/// @brief Frame 测试基建：临时 XML 文件 + 预注入的 MapPropertyContext。
struct FrameFixture {
    QTemporaryFile file;                                    ///< 布局临时文件。
    std::shared_ptr<MapPropertyContext> ctx = std::make_shared<MapPropertyContext>();  ///< 属性上下文。

    /// @brief 落盘布局并构造 Frame（构造期完成首次物化/布局）。
    std::unique_ptr<Frame> make(const QString& inner)
    {
        file.setFileTemplate(QDir::tempPath() + QStringLiteral("/bi_diag_XXXXXX.xml"));
        if (!file.open())
            return nullptr;
        file.write(wrap(inner).toUtf8());
        file.flush();
        return Frame::fromFile(file.fileName(), ctx);
    }
};

/// @brief Q_PROPERTY 测试目标：只读 device 属性（BI-R-006 触发源）。
class DiagTestObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap device READ device)
public:
    QVariantMap device() const { return m_device; }
    void setDevice(const QVariantMap& m) { m_device = m; }

private:
    QVariantMap m_device;
};

class TestDiagnostics : public QObject {
    Q_OBJECT

private slots:
    // ========== 解析期：21 个错误码 ==========

    /// @brief BI-P-001：文件无法打开 → Abort。
    void fileOpenFails()
    {
        CaptureCollector cap;
        const QString path = QStringLiteral("/nonexistent/bi_diag_x.xml");
        auto root = XmlLayoutParser::parseFile(path, &cap);
        QVERIFY(root == nullptr);
        const Diagnostic* d = cap.find(ErrorCode::FileOpenFailed);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Error);
        QCOMPARE(recoveryOf(d->code), Recovery::Abort);
        QCOMPARE(d->file, path);
    }

    /// @brief BI-P-002：XML 语法错误 → Abort，携带行列号。
    void xmlSyntaxError()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(QStringLiteral("<root><text></root>"), &cap);
        QVERIFY(root == nullptr);
        const Diagnostic* d = cap.find(ErrorCode::XmlSyntaxError);
        QVERIFY(d != nullptr);
        QVERIFY(d->line > 0);
        QVERIFY(d->column > 0);
    }

    /// @brief BI-P-003：根元素非 <root> → Abort。
    void rootNotRoot()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(QStringLiteral("<notroot/>"), &cap);
        QVERIFY(root == nullptr);
        QVERIFY(cap.find(ErrorCode::RootNotRoot) != nullptr);
    }

    /// @brief BI-P-004：<root> 子元素数 ≠ 1（0 个 / 2 个各一条）。
    void rootChildCount()
    {
        CaptureCollector cap;
        QVERIFY(XmlLayoutParser::parseString(QStringLiteral("<root/>"), &cap) == nullptr);
        QVERIFY(XmlLayoutParser::parseString(
                    QStringLiteral("<root><text/><text/></root>"), &cap) == nullptr);
        QCOMPARE(cap.count(ErrorCode::RootChildCount), 2);
    }

    /// @brief BI-P-005：未知元素标签 → Error/Abort（级别正名），元素路径定位。
    void unknownElementTag()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(wrap(QStringLiteral("<label/>")), &cap);
        QVERIFY(root == nullptr);
        const Diagnostic* d = cap.find(ErrorCode::UnknownElementTag);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Error);
        QCOMPARE(d->elementPath, QStringLiteral("root/label"));
    }

    /// @brief BI-P-006：deprecated 装饰器标签 → Warning/Default。
    void deprecatedDecoratorTag()
    {
        CaptureCollector cap;
        XmlLayoutParser::parseString(wrap(QStringLiteral("<margin/>")), &cap);
        const Diagnostic* d = cap.find(ErrorCode::DeprecatedDecoratorTag);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Warning);
        QCOMPARE(recoveryOf(d->code), Recovery::Default);
    }

    /// @brief BI-P-022：根唯一子元素降级为空 → 补报 Abort，维持"nullptr ⇔ Abort"不变量。
    void rootChildDiscardedYieldsAbort()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(wrap(QStringLiteral("<margin/>")), &cap);
        QVERIFY(root == nullptr);                        // 无可用内容，整文件失败
        QVERIFY(cap.find(ErrorCode::DeprecatedDecoratorTag) != nullptr);  // 原降级码仍在
        const Diagnostic* d = cap.find(ErrorCode::RootChildDiscarded);
        QVERIFY(d != nullptr);                           // 不变量守卫补报的 Abort
        QCOMPARE(severityOf(d->code), Severity::Error);
        QCOMPARE(recoveryOf(d->code), Recovery::Abort);
    }

    /// @brief BI-P-007：不可含子元素的部件含子元素 → Abort。
    void leafWithChildren()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<text><text>nested</text></text>")), &cap);
        QVERIFY(root == nullptr);
        QVERIFY(cap.find(ErrorCode::LeafWithChildren) != nullptr);
    }

    /// @brief BI-P-008：grid 直接子元素非 <cell> → Abort。
    void gridNonCellChild()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<grid columns=\"1\" rows=\"1\"><text/></grid>")), &cap);
        QVERIFY(root == nullptr);
        QVERIFY(cap.find(ErrorCode::GridNonCellChild) != nullptr);
    }

    /// @brief BI-P-009：grid cell 数 ≠ columns×rows → Abort。
    void gridCellCountMismatch()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<grid columns=\"1\" rows=\"1\"><cell/><cell/></grid>")), &cap);
        QVERIFY(root == nullptr);
        QVERIFY(cap.find(ErrorCode::GridCellCountMismatch) != nullptr);
    }

    /// @brief BI-P-010：<if> 缺 b:prop → Abort。
    void ifMissingProp()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<if><text>x</text></if>")), &cap);
        QVERIFY(root == nullptr);
        QVERIFY(cap.find(ErrorCode::IfMissingProp) != nullptr);
    }

    /// @brief BI-P-011：未知属性 → Warning/Default，parse 成功。
    void unknownAttribute()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<text foo=\"1\">x</text>")), &cap);
        QVERIFY(root != nullptr);
        const Diagnostic* d = cap.find(ErrorCode::UnknownAttribute);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Warning);
    }

    /// @brief BI-P-012：未知绑定属性 → Warning，parse 成功。
    void unknownBindingAttribute()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<text b:foo=\"x\">x</text>")), &cap);
        QVERIFY(root != nullptr);
        QVERIFY(cap.find(ErrorCode::UnknownBindingAttribute) != nullptr);
    }

    /// @brief BI-P-023：布局策略属性不参与绑定（grid 的 b:columns）→ Warning，拒绝注册。
    void bindingNotSupported()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<grid rows=\"1\" b:columns=\"n\"><cell/></grid>")), &cap);
        QVERIFY(root != nullptr);
        const Diagnostic* d = cap.find(ErrorCode::BindingNotSupported);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Warning);
        QCOMPARE(recoveryOf(d->code), Recovery::Default);
        // 解析期拒绝：绑定未注册，bindsProperty 不命中
        QVERIFY(!root->bindsProperty(QStringLiteral("n")));
    }

    /// @brief BI-P-023：布局策略属性不参与绑定（text 的 b:wrap）→ Warning，拒绝注册。
    void bindingNotSupportedText()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<text b:wrap=\"w\">x</text>")), &cap);
        QVERIFY(root != nullptr);
        QVERIFY(cap.find(ErrorCode::BindingNotSupported) != nullptr);
        QVERIFY(!root->bindsProperty(QStringLiteral("w")));
    }

    /// @brief BI-P-023 全量覆盖：一个 XML 装齐全部 19 处合法但不可绑定的布局策略属性
    ///        （column 4 + row 4 + text 4 + grid 5 + cell 2），断言收集器逐条收齐。
    void bindingNotSupportedExhaustive()
    {
        CaptureCollector cap;
        const QString inner = QStringLiteral(
            "<column b:main-align=\"a\" b:cross-align=\"b\" b:space=\"c\" b:main-stretch=\"d\">"
            "<row b:main-align=\"e\" b:cross-align=\"f\" b:space=\"g\" b:main-stretch=\"h\">"
            "<text b:v-align=\"i\" b:h-align=\"j\" b:wrap=\"k\" b:max-width=\"l\">x</text>"
            "</row>"
            "<grid b:columns=\"m\" b:rows=\"n\" b:space=\"o\" b:space-row=\"p\" b:space-column=\"q\">"
            "<cell b:v-align=\"r\" b:h-align=\"s\"><text>y</text></cell>"
            "</grid>"
            "</column>");
        auto root = XmlLayoutParser::parseString(wrap(inner), &cap);
        QVERIFY(root != nullptr);
        // 全收集语义：19 处逐一报告，不多不少
        QCOMPARE(cap.count(ErrorCode::BindingNotSupported), 19);
        // 每个被拒绝的属性限定名都应出现在某条诊断消息中（12 个去重名）
        const QStringList attrs = {QStringLiteral("b:main-align"), QStringLiteral("b:cross-align"),
                                   QStringLiteral("b:space"), QStringLiteral("b:main-stretch"),
                                   QStringLiteral("b:v-align"), QStringLiteral("b:h-align"),
                                   QStringLiteral("b:wrap"), QStringLiteral("b:max-width"),
                                   QStringLiteral("b:columns"), QStringLiteral("b:rows"),
                                   QStringLiteral("b:space-row"), QStringLiteral("b:space-column")};
        for (const QString& a : attrs) {
            bool found = false;
            for (const auto& d : cap.list) {
                if (d.code == ErrorCode::BindingNotSupported && d.message.contains(a)) {
                    found = true;
                    break;
                }
            }
            QVERIFY2(found, qPrintable(a));
        }
    }

    /// @brief BI-P-014：content 与 b:content 互斥 → Error/Default，字面量生效。
    void mutexLiteralWins()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<text content=\"lit\" b:content=\"msg\">x</text>")), &cap);
        QVERIFY(root != nullptr);
        const Diagnostic* d = cap.find(ErrorCode::MutexLiteralBinding);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Error);
        QCOMPARE(recoveryOf(d->code), Recovery::Default);

        MapPropertyContext map;
        map.setProperty(QStringLiteral("msg"), QStringLiteral("bound"));
        LayoutContext lctx{&map};
        auto nodes = root->materializeChildren(lctx);
        QVERIFY(!nodes.empty());
        QCOMPARE(static_cast<const TextNode&>(*nodes[0]).text, QStringLiteral("lit"));
    }

    /// @brief BI-P-015：字面量类型错误（font-size / main-stretch 布尔）→ Default 回退。
    ///
    /// 回归点：校验失败不得覆写属性成员——font-size="abc" 解析后 m_fontSize
    /// 保持默认 12，物化期不得再产生伪 BI-R-011（运行时诊断走进程级收集器）。
    void literalTypeMismatch()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<text font-size=\"abc\">x</text>")), &cap);
        QVERIFY(root != nullptr);
        QVERIFY(cap.find(ErrorCode::LiteralTypeMismatch) != nullptr);

        auto rtCap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(rtCap);
        MapPropertyContext map;
        LayoutContext lctx{&map};
        auto nodes = root->materializeChildren(lctx);
        QVERIFY(!nodes.empty());
        QCOMPARE(static_cast<const TextNode&>(*nodes[0]).fontSize, 12.0);
        QCOMPARE(rtCap->list.size(), 0);  // 无伪运行时诊断（解析期错误不重复到运行时）

        CaptureCollector cap2;
        auto root2 = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<column main-stretch=\"yes\"><text>x</text></column>")), &cap2);
        QVERIFY(root2 != nullptr);
        QVERIFY(cap2.find(ErrorCode::LiteralTypeMismatch) != nullptr);
    }

    /// @brief BI-P-015（validateInt 路径）：columns="abc" 回退默认 1，
    /// 不得级联为 BI-P-009 Abort（Warning/Error 级输入不得放大为整文件失败）。
    void literalIntTypeMismatchKeepsDefault()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<grid columns=\"abc\" rows=\"1\"><cell/></grid>")), &cap);
        QVERIFY(root != nullptr);  // columns 保持默认 1 → cell 数 1×1 匹配，无级联 Abort
        QVERIFY(cap.find(ErrorCode::LiteralTypeMismatch) != nullptr);
        QVERIFY(cap.find(ErrorCode::GridCellCountMismatch) == nullptr);
        auto grid = std::dynamic_pointer_cast<GridLayout>(root);
        QVERIFY(grid != nullptr);
        QCOMPARE(grid->columns(), 1);
    }

    /// @brief BI-P-016：字面量数值越界（font-size≤0、columns≤0）→ 钳到默认。
    void literalOutOfRange()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<text font-size=\"-3\">x</text>")), &cap);
        QVERIFY(root != nullptr);
        QVERIFY(cap.find(ErrorCode::LiteralOutOfRange) != nullptr);

        MapPropertyContext map;
        LayoutContext lctx{&map};
        auto nodes = root->materializeChildren(lctx);
        QVERIFY(!nodes.empty());
        QCOMPARE(static_cast<const TextNode&>(*nodes[0]).fontSize, 12.0);

        CaptureCollector cap2;
        auto root2 = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<grid columns=\"0\" rows=\"1\"><cell/></grid>")), &cap2);
        QVERIFY(root2 != nullptr);  // columns 钳到 1，cell 数 1×1 匹配
        QVERIFY(cap2.find(ErrorCode::LiteralOutOfRange) != nullptr);
    }

    /// @brief BI-P-018：b:as 无 b:of → Warning，parse 成功。
    void asWithoutOf()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<text b:as=\"x\">hi</text>")), &cap);
        QVERIFY(root != nullptr);
        QVERIFY(cap.find(ErrorCode::AsWithoutOf) != nullptr);
    }

    /// @brief BI-P-019：<if> 的 b:not 绑定 → Warning，not 不生效。
    void notBindingIgnored()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<if b:prop=\"s\" b:not=\"x\"><text>v</text></if>")), &cap);
        QVERIFY(root != nullptr);
        QVERIFY(cap.find(ErrorCode::NotBindingIgnored) != nullptr);
    }

    /// @brief BI-P-020：Registry 目录不存在 → Warning，返回 0。
    void registryDirMissing()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        const int n = LayoutRegistry::instance().loadLayoutsFromDirectory(
            QStringLiteral("/nonexistent/bi_diag_dir"));
        QCOMPARE(n, 0);
        const Diagnostic* d = cap->find(ErrorCode::RegistryDirMissing);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Warning);
    }

    /// @brief BI-P-021：Registry 批量加载跳过坏文件 → 返回成功数，坏文件诊断聚合。
    void registrySkipsBadFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile good(dir.filePath(QStringLiteral("good.xml")));
        QVERIFY(good.open(QIODevice::WriteOnly));
        good.write(wrap(QStringLiteral("<text>ok</text>")).toUtf8());
        good.close();
        QFile bad(dir.filePath(QStringLiteral("bad.xml")));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write(wrap(QStringLiteral("<label/>")).toUtf8());
        bad.close();

        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        const int n = LayoutRegistry::instance().loadLayoutsFromDirectory(dir.path());
        QCOMPARE(n, 1);
        QVERIFY(cap->find(ErrorCode::UnknownElementTag) != nullptr);
        QVERIFY(cap->find(ErrorCode::RegistryFileSkipped) != nullptr);
    }

    // ========== 运行时：11 个错误码 ==========

    /// @brief BI-R-001：绑定路径首段属性在 QObject 上不存在。
    void objectFirstKeyMissing()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        DiagTestObject obj;
        QPropertyContext ctx(&obj);
        ctx.property(QStringLiteral("foo.bar"));
        QVERIFY(cap->find(ErrorCode::ObjectFirstKeyMissing) != nullptr);
    }

    /// @brief BI-R-002：路径中段键在 map 中不存在。
    void nestedKeyMissing()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        MapPropertyContext map;
        map.setProperty(QStringLiteral("a"), QVariantMap{});
        map.property(QStringLiteral("a.b"));
        QVERIFY(cap->find(ErrorCode::NestedKeyMissing) != nullptr);
    }

    /// @brief BI-R-003：路径遍历类型不符（对非 map 取键）。
    void typeMismatchTraverse()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        MapPropertyContext map;
        map.setProperty(QStringLiteral("a"), QStringLiteral("s"));
        map.property(QStringLiteral("a.b"));
        QVERIFY(cap->find(ErrorCode::TraverseTypeMismatch) != nullptr);
    }

    /// @brief BI-R-004：数组下标越界。
    void indexOutOfBounds()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        MapPropertyContext map;
        map.setProperty(QStringLiteral("a"), QVariantList{1, 2});
        map.property(QStringLiteral("a[5]"));
        QVERIFY(cap->find(ErrorCode::IndexOutOfBounds) != nullptr);
    }

    /// @brief BI-R-005：路径语法错误（未闭合 '['）。
    void pathSyntaxError()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        MapPropertyContext map;
        map.setProperty(QStringLiteral("a"), QStringList{QStringLiteral("x")});
        map.property(QStringLiteral("a["));
        QVERIFY(cap->find(ErrorCode::PathSyntaxError) != nullptr);
    }

    /// @brief BI-R-006：setProperty 类型不匹配（只读 Q_PROPERTY 拒绝写入）。
    void setPropertyTypeMismatch()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        DiagTestObject obj;
        obj.setDevice(QVariantMap{{QStringLiteral("cpu"), QStringLiteral("1%")}});
        QPropertyContext ctx(&obj);
        ctx.setProperty(QStringLiteral("device"), QStringLiteral("x"));
        const Diagnostic* d = cap->find(ErrorCode::SetPropertyTypeMismatch);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Warning);
    }

    /// @brief BI-R-007：<for> 缺 b:as（物化期暴露）→ 迭代产出空。
    void forMissingAs()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        FrameFixture fx;
        fx.ctx->setProperty(QStringLiteral("items"), QStringList{QStringLiteral("a")});
        auto frame = fx.make(QStringLiteral(
            "<column><for b:of=\"items\"><text b:content=\"i\"/></for></column>"));
        QVERIFY(frame != nullptr);
        QVERIFY(cap->find(ErrorCode::ForMissingAs) != nullptr);
        QCOMPARE(frame->size(), QSizeF(0, 0));  // 迭代产出空 → 整帧 0×0
    }

    /// @brief BI-R-008：<for> 数据源非列表 → 视为空列表。
    void forDataNotList()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        FrameFixture fx;
        fx.ctx->setProperty(QStringLiteral("items"), QStringLiteral("notalist"));
        auto frame = fx.make(QStringLiteral(
            "<column><for b:of=\"items\" b:as=\"i\"><text b:content=\"i\"/></for></column>"));
        QVERIFY(frame != nullptr);
        QVERIFY(cap->find(ErrorCode::ForDataNotList) != nullptr);
    }

    /// @brief BI-R-009：<for> 列表含 null 项 → Warning/Skip，其余照常。
    void forNullItemSkipped()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        FrameFixture fx;
        fx.ctx->setProperty(QStringLiteral("items"),
                            QVariantList{QStringLiteral("a"), QVariant()});
        auto frame = fx.make(QStringLiteral(
            "<column><for b:of=\"items\" b:as=\"i\"><text b:content=\"i\"/></for></column>"));
        QVERIFY(frame != nullptr);
        const Diagnostic* d = cap->find(ErrorCode::ForNullItemSkipped);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Warning);
        QVERIFY(frame->size().height() > 0);  // 有效项照常物化
    }

    /// @brief BI-R-010：<image> 加载失败 → Warning，空白占位。
    void imageLoadFails()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        FrameFixture fx;
        fx.ctx->setProperty(QStringLiteral("icon"), QStringLiteral("/nonexistent/bi_diag.png"));
        auto frame = fx.make(QStringLiteral("<image b:src=\"icon\"/>"));
        QVERIFY(frame != nullptr);
        const Diagnostic* d = cap->find(ErrorCode::ImageLoadFailed);
        QVERIFY(d != nullptr);
        QCOMPARE(severityOf(d->code), Severity::Warning);
    }

    /// @brief BI-R-011：绑定值类型错误（b:color 绑到 int）→ 回退默认。
    void boundValueTypeError()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        FrameFixture fx;
        fx.ctx->setProperty(QStringLiteral("c"), 123);
        auto frame = fx.make(QStringLiteral("<text b:color=\"c\">x</text>"));
        QVERIFY(frame != nullptr);
        QVERIFY(cap->find(ErrorCode::BoundValueTypeError) != nullptr);
    }

    // ========== 行为级用例 ==========

    /// @brief 嵌套 Abort 整文件失败：bad grid 嵌在 column 内 → root nullptr（旧行为为裁剪成功）。
    void nestedAbortFailsWholeFile()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(wrap(QStringLiteral(
            "<column><grid columns=\"1\" rows=\"1\"><text/></grid><text>ok</text></column>")), &cap);
        QVERIFY(root == nullptr);
        const Diagnostic* d = cap.find(ErrorCode::GridNonCellChild);
        QVERIFY(d != nullptr);
        QCOMPARE(d->elementPath, QStringLiteral("root/column/grid"));
    }

    /// @brief 全收集：一个文件两处错误 → 收集器恰收两条，parse 仍按契约失败。
    void collectAllReportsBoth()
    {
        CaptureCollector cap;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<column><label/><text foo=\"1\">x</text></column>")), &cap);
        QVERIFY(root == nullptr);
        QCOMPARE(cap.count(ErrorCode::UnknownElementTag), 1);
        QCOMPARE(cap.count(ErrorCode::UnknownAttribute), 1);
    }

    /// @brief 运行时模板级去重：连发重布局只报一次；无 scope 直接物化照报。
    void runtimeDedupOncePerTemplate()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);

        FrameFixture fx;
        fx.ctx->setProperty(QStringLiteral("a"), QVariantMap{});
        auto frame = fx.make(QStringLiteral("<text b:content=\"a.b\"/>"));
        QVERIFY(frame != nullptr);
        QCOMPARE(cap->count(ErrorCode::NestedKeyMissing), 1);  // 构造期首次报告

        // 换值触发重布局（同值会被变更检测跳过）：第二次遍历被去重抑制
        fx.ctx->setProperty(QStringLiteral("a"), QVariantMap{{QStringLiteral("x"), 1}});
        frame->flush();
        QCOMPARE(cap->count(ErrorCode::NestedKeyMissing), 1);

        // 无 RuntimeScope（直接物化）：不去重，照报
        CaptureCollector cap2;
        auto root = XmlLayoutParser::parseString(
            wrap(QStringLiteral("<text b:content=\"a.b\"/>")), &cap2);
        QVERIFY(root != nullptr);
        MapPropertyContext map;
        map.setProperty(QStringLiteral("a"), QVariantMap{});
        LayoutContext lctx{&map};
        root->materializeChildren(lctx);
        root->materializeChildren(lctx);
        QCOMPARE(cap->count(ErrorCode::NestedKeyMissing), 3);  // 1（Frame）+ 2（无 scope）
    }

    /// @brief 静默清单：绑定属性未注入、<if> 存在性落空 → 零诊断。
    void silentCases()
    {
        auto cap = std::make_shared<CaptureCollector>();
        const ProcessCollectorGuard guard(cap);
        FrameFixture fx;
        auto frame = fx.make(QStringLiteral(
            "<column><text b:content=\"missing\"/><if b:prop=\"ghost\"><text>v</text></if></column>"));
        QVERIFY(frame != nullptr);
        QCOMPARE(cap->list.size(), 0);
    }
};

QTEST_MAIN(TestDiagnostics)
#include "test_diagnostics.moc"
