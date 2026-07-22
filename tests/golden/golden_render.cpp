/**
 * @file golden_render.cpp
 * @brief 黄金镜像采集/校验工具。
 *
 * 用法：
 *   golden_render --capture <dir>  按内置 manifest 渲染 10 张 PNG 到 <dir>。
 *   golden_render --verify <dir>   重新渲染并与 <dir> 中的黄金 PNG 逐像素比对。
 *
 * verify 语义：等价组（intentionalChange=false）必须像素完全一致，否则退出码 1；
 * 有意变更组（intentionalChange=true）只打印 MATCH/DIFF，不影响退出码。
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>
#include <QTemporaryFile>
#include <QDebug>

#include <broaditem/core/Frame.h>
#include <broaditem/core/BroadItem.h>

namespace {

/// @brief 单条黄金镜像语料条目。
struct GoldenCase {
    QString name;             ///< 条目名（同时作为 PNG 文件名）。
    QString xmlPath;          ///< XML 布局文件路径（相对仓库根目录）；以 "inline:" 前缀开头时表示内联 XML 内容。
    QVariantMap props;        ///< 需要注入的动态属性。
    bool intentionalChange;   ///< true 表示该条目为有意变更组，不参与 verify 失败判定。
    bool useScene;            ///< true 表示走 QGraphicsScene + BroadItem 路径，否则走 Frame 路径。
};

/// @brief 构造 complex 语料的 processes 属性（三条进程记录）。
static QVariantList makeProcesses()
{
    QVariantMap p1; p1.insert(QStringLiteral("name"), QStringLiteral("com.apple.WebKit.WebContent"));
    p1.insert(QStringLiteral("pid"), 87471); p1.insert(QStringLiteral("cpu"), 12.3);
    QVariantMap p2; p2.insert(QStringLiteral("name"), QStringLiteral("kernel_task"));
    p2.insert(QStringLiteral("pid"), 0); p2.insert(QStringLiteral("cpu"), 4.5);
    QVariantMap p3; p3.insert(QStringLiteral("name"), QStringLiteral("WindowServer"));
    p3.insert(QStringLiteral("pid"), 199); p3.insert(QStringLiteral("cpu"), 3.1);
    return { p1, p2, p3 };
}

/// @brief 构造 control 语料的 users 属性。
static QVariantList makeUsers()
{
    QVariantMap u1; u1.insert(QStringLiteral("name"), QStringLiteral("Alice"));
    u1.insert(QStringLiteral("email"), QStringLiteral("a@x.com"));
    QVariantMap u2; u2.insert(QStringLiteral("name"), QStringLiteral("Bob"));
    u2.insert(QStringLiteral("email"), QStringLiteral("b@x.com"));
    return { u1, u2 };
}

/// @brief 构造 control 语料的 groups 属性（嵌套 members 字符串列表）。
static QVariantList makeGroups()
{
    QVariantMap g1; g1.insert(QStringLiteral("name"), QStringLiteral("第一组"));
    g1.insert(QStringLiteral("members"), QStringList{ QStringLiteral("甲"), QStringLiteral("乙") });
    QVariantMap g2; g2.insert(QStringLiteral("name"), QStringLiteral("第二组"));
    g2.insert(QStringLiteral("members"), QStringList{ QStringLiteral("丙") });
    return { g1, g2 };
}

/// @brief 构造 basic 语料属性（broaditem_basic 复用）。
static QVariantMap makeBasicProps()
{
    return {
        { QStringLiteral("title"), QStringLiteral("设备状态") },
        { QStringLiteral("status"), QStringLiteral("运行中") },
        { QStringLiteral("ip"), QStringLiteral("192.168.1.100") },
    };
}

/// @brief 构造 control_show 语料属性（control_hide 在此基础上去掉 extra）。
static QVariantMap makeControlProps(bool withExtra)
{
    QVariantMap props{
        { QStringLiteral("title"), QStringLiteral("控制元素") },
        { QStringLiteral("items"), QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta"), QStringLiteral("Gamma") } },
        { QStringLiteral("users"), makeUsers() },
        { QStringLiteral("groups"), makeGroups() },
    };
    if (withExtra)
        props.insert(QStringLiteral("extra"), QStringLiteral("额外信息"));
    return props;
}

/// @brief 返回硬编码的 10 条黄金镜像 manifest。
static QVector<GoldenCase> manifest()
{
    QVariantMap complexProps{
        { QStringLiteral("device_name"), QStringLiteral("服务器 #01") },
        { QStringLiteral("status_text"), QStringLiteral("运行中") },
        { QStringLiteral("cpu"), QStringLiteral("45%") },
        { QStringLiteral("memory"), QStringLiteral("62%") },
        { QStringLiteral("disk"), QStringLiteral("78%") },
        { QStringLiteral("net_in"), QStringLiteral("12.5 MB/s") },
        { QStringLiteral("net_out"), QStringLiteral("3.2 MB/s") },
        { QStringLiteral("temp"), QStringLiteral("58°C") },
        { QStringLiteral("ipv4"), QStringLiteral("192.168.1.100") },
        { QStringLiteral("mac"), QStringLiteral("AA:BB:CC:DD:EE:FF") },
        { QStringLiteral("gateway"), QStringLiteral("192.168.1.1") },
        { QStringLiteral("dns"), QStringLiteral("8.8.8.8") },
        { QStringLiteral("warning"), QStringLiteral("CPU 使用率超过阈值！") },
        { QStringLiteral("processes"), makeProcesses() },
    };

    QVariantMap playgroundProps{
        { QStringLiteral("title"), QStringLiteral("系统状态监控") },
        { QStringLiteral("cpu"), QStringLiteral("45%") },
        { QStringLiteral("memory"), QStringLiteral("2.3GB / 8GB") },
    };

    QVariantMap cellForProps{
        { QStringLiteral("lines"), QStringList{ QStringLiteral("第一行"), QStringLiteral("第二行"), QStringLiteral("第三行") } },
    };

    return {
        { QStringLiteral("basic"), QStringLiteral("example/basic/layout.xml"), makeBasicProps(), false, false },
        { QStringLiteral("complex"), QStringLiteral("example/complex/layout.xml"), complexProps, false, false },
        { QStringLiteral("playground"), QStringLiteral("example/playground/layout.xml"), playgroundProps, false, false },
        { QStringLiteral("main_stretch_with"), QStringLiteral("example/main_stretch/main_stretch_with.xml"), {}, false, false },
        { QStringLiteral("main_stretch_without"), QStringLiteral("example/main_stretch/main_stretch_without.xml"), {}, false, false },
        { QStringLiteral("control_show"), QStringLiteral("tests/golden/layouts/control.xml"), makeControlProps(true), true, false },
        { QStringLiteral("control_hide"), QStringLiteral("tests/golden/layouts/control.xml"), makeControlProps(false), true, false },
        { QStringLiteral("cell_for"), QStringLiteral("tests/golden/layouts/cell_for.xml"), cellForProps, true, false },
        { QStringLiteral("broaditem_basic"), QStringLiteral("example/basic/layout.xml"), makeBasicProps(), false, true },
        { QStringLiteral("image_icon"), QStringLiteral("inline:<root xmlns:b='urn:broaditem:binding'><row space='8' padding='12' background-color='#ffffff'><image src='tests/assets/red.png' width='16' height='16'/><text font-size='12' color='#333'>图标</text></row></root>"), {}, false, false },
    };
}

/// @brief 通过 Frame 路径渲染条目为 QImage。
/// @param c 语料条目。
/// @return 渲染结果；失败返回空 QImage。
static QImage renderWithFrame(const GoldenCase& c)
{
    auto frame = BroadItem::Frame::fromFile(c.xmlPath);
    for (auto it = c.props.constBegin(); it != c.props.constEnd(); ++it)
        frame->setDynamicProperty(it.key(), it.value());
    return frame->toImage(1.0);
}

/// @brief 通过 QGraphicsScene + BroadItem 路径渲染条目为 QImage。
/// @param c 语料条目。
/// @return 渲染结果；失败返回空 QImage。
static QImage renderWithScene(const GoldenCase& c)
{
    QGraphicsScene scene;
    auto* item = new BroadItem::BroadItem(c.xmlPath);
    for (auto it = c.props.constBegin(); it != c.props.constEnd(); ++it)
        item->setDynamicProperty(it.key(), it.value());
    scene.addItem(item);

    const QRectF source = item->boundingRect();
    QImage image(source.size().toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter, QRectF(), source);
    painter.end();
    return image;
}

/// @brief 按条目配置渲染一张图。
///
/// xmlPath 以 "inline:" 前缀开头时，先将其余内容落盘为临时 XML 文件再渲染；
/// 临时文件生命周期覆盖整个渲染调用（fromFile 解析 + toImage 物化），
/// 返回后即自动删除。
static QImage renderCase(const GoldenCase& c)
{
    const QString inlinePrefix = QStringLiteral("inline:");
    if (!c.xmlPath.startsWith(inlinePrefix))
        return c.useScene ? renderWithScene(c) : renderWithFrame(c);

    QTemporaryFile inlineFile(QDir::tempPath() + QStringLiteral("/broaditem_golden_XXXXXX.xml"));
    if (!inlineFile.open()) {
        qCritical() << "[FAIL]" << c.name << "无法创建内联 XML 临时文件";
        return {};
    }
    inlineFile.write(c.xmlPath.mid(inlinePrefix.size()).toUtf8());
    inlineFile.flush();

    GoldenCase resolved = c;
    resolved.xmlPath = inlineFile.fileName();
    return resolved.useScene ? renderWithScene(resolved) : renderWithFrame(resolved);
}

/// @brief 把透明背景的渲染结果合成到不透明白底上。
///
/// 半透明像素在 premultiply/unpremultiply 与 PNG 往返间存在 ±1 舍入漂移，
/// 合成到不透明的 RGB32 后保存/加载完全无损，保证同代码自比严格幂等。
/// @param src 原始渲染图（可能带透明通道）。
/// @return 白底不透明图（Format_RGB32）。
static QImage flattenToOpaque(const QImage& src)
{
    QImage dst(src.size(), QImage::Format_RGB32);
    dst.fill(Qt::white);
    QPainter painter(&dst);
    painter.drawImage(0, 0, src);
    painter.end();
    return dst;
}

/// @brief 像素比对结果。
struct CompareResult {
    bool sizeMismatch = false;   ///< 尺寸是否不一致。
    int diffCount = 0;           ///< 不同像素数量。
    QRect diffBounds;            ///< 不同像素的包围盒（无差异时为空）。
};

/// @brief 逐像素比较两张图（容差 0）。
/// @param actual   实际渲染图。
/// @param expected 黄金参考图。
/// @return 比较结果。
static CompareResult compareImages(const QImage& actual, const QImage& expected)
{
    CompareResult result;
    if (actual.size() != expected.size()) {
        result.sizeMismatch = true;
        return result;
    }

    const int w = actual.width();
    const int h = actual.height();
    int minX = w, minY = h, maxX = -1, maxY = -1;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (actual.pixel(x, y) != expected.pixel(x, y)) {
                ++result.diffCount;
                minX = qMin(minX, x); minY = qMin(minY, y);
                maxX = qMax(maxX, x); maxY = qMax(maxY, y);
            }
        }
    }
    if (result.diffCount > 0)
        result.diffBounds = QRect(QPoint(minX, minY), QPoint(maxX, maxY));
    return result;
}

/// @brief 预热渲染：正式采集/校验前把每条语料渲染一遍并丢弃结果。
///
/// 文本首次渲染会触发 Qt 字体数据库/ shaping 的懒初始化，初始化前后的
/// 字体度量可能不同，导致同代码在不同进程中布局尺寸漂移。预热后进程内
/// 字体状态收敛，正式渲染结果确定。
static void warmupRender()
{
    const QVector<GoldenCase> cases = manifest();
    for (const GoldenCase& c : cases) {
        const QImage discarded = renderCase(c);
        Q_UNUSED(discarded);
    }
}

/// @brief 采集模式：渲染全部条目并保存 PNG 到输出目录。
/// @param outDir 输出目录。
/// @return 成功返回 0，否则返回 1。
static int capture(const QString& outDir)
{
    QDir dir(outDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qCritical() << "无法创建输出目录:" << outDir;
        return 1;
    }

    warmupRender();
    for (const GoldenCase& c : manifest()) {
        const QImage image = flattenToOpaque(renderCase(c));
        if (image.isNull()) {
            qCritical() << "[FAIL]" << c.name << "渲染失败";
            return 1;
        }
        const QString path = dir.filePath(c.name + QStringLiteral(".png"));
        if (!image.save(path)) {
            qCritical() << "[FAIL]" << c.name << "保存失败:" << path;
            return 1;
        }
        qInfo().noquote() << QStringLiteral("[CAPTURE] %1.png %2x%3")
            .arg(c.name).arg(image.width()).arg(image.height());
    }
    return 0;
}

/// @brief 校验模式：重新渲染并与黄金 PNG 比对。
/// @param goldenDir 黄金 PNG 所在目录。
/// @return 等价组全部 MATCH 返回 0，否则返回 1。
static int verify(const QString& goldenDir)
{
    QDir dir(goldenDir);
    bool failed = false;

    warmupRender();
    for (const GoldenCase& c : manifest()) {
        const QString path = dir.filePath(c.name + QStringLiteral(".png"));
        const QImage expected(path);
        if (expected.isNull()) {
            qCritical() << "[ERROR]" << c.name << "无法加载黄金图:" << path;
            if (!c.intentionalChange)
                failed = true;
            continue;
        }

        const QImage actual = flattenToOpaque(renderCase(c));
        if (actual.isNull()) {
            qCritical() << "[ERROR]" << c.name << "渲染失败";
            if (!c.intentionalChange)
                failed = true;
            continue;
        }

        const CompareResult cmp = compareImages(actual, expected);
        const bool match = !cmp.sizeMismatch && cmp.diffCount == 0;
        const QString tag = c.intentionalChange ? QStringLiteral("intentional") : QStringLiteral("equivalence");

        if (cmp.sizeMismatch) {
            qInfo().noquote() << QStringLiteral("[DIFF] %1 (%2) 尺寸不一致 actual=%3x%4 expected=%5x%6")
                .arg(c.name, tag).arg(actual.width()).arg(actual.height())
                .arg(expected.width()).arg(expected.height());
        } else if (match) {
            qInfo().noquote() << QStringLiteral("[MATCH] %1 (%2) %3x%4")
                .arg(c.name, tag).arg(actual.width()).arg(actual.height());
        } else {
            const QRect b = cmp.diffBounds;
            qInfo().noquote() << QStringLiteral("[DIFF] %1 (%2) diff像素数=%3 包围盒=(%4,%5 %6x%7)")
                .arg(c.name, tag).arg(cmp.diffCount)
                .arg(b.x()).arg(b.y()).arg(b.width()).arg(b.height());
        }

        if (!match && !c.intentionalChange)
            failed = true;
    }

    return failed ? 1 : 0;
}

} // namespace

/// @brief 程序入口：解析命令行并分派 capture/verify。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("golden_render"));

    // 固定应用默认字体：macOS 系统字体 .AppleSystemUIFont 的 bold 变体匹配
    // 在进程间不稳定（真实 bold 与合成 bold 两种状态交替，行高相差数像素），
    // 导致含 bold 文本的布局跨进程漂移。显式指定 PingFang SC 后字体匹配确定。
    QApplication::setFont(QFont(QStringLiteral("PingFang SC")));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("BroadItem 黄金镜像采集/校验工具"));
    parser.addHelpOption();
    const QCommandLineOption captureOpt(QStringLiteral("capture"), QStringLiteral("采集黄金 PNG 到 <dir>"), QStringLiteral("dir"));
    const QCommandLineOption verifyOpt(QStringLiteral("verify"), QStringLiteral("与 <dir> 中黄金 PNG 校验"), QStringLiteral("dir"));
    parser.addOption(captureOpt);
    parser.addOption(verifyOpt);
    parser.process(app);

    if (parser.isSet(captureOpt) && parser.isSet(verifyOpt)) {
        qCritical() << "--capture 与 --verify 不能同时使用";
        return 2;
    }
    if (parser.isSet(captureOpt))
        return capture(parser.value(captureOpt));
    if (parser.isSet(verifyOpt))
        return verify(parser.value(verifyOpt));

    parser.showHelp(2);
}
