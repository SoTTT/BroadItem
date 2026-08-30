#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QImage>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/element/Element.h>
#include <broaditem/element/image/ImageElement.h>
#include <broaditem/core/Node.h>
#include <broaditem/core/Frame.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>

#include "helpers/binding_helpers.h"
#include "helpers/frame_helpers.h"

using namespace BroadItem;
using namespace BroadItem::TestHelpers;

/// @brief 生成纯色 PNG 到指定路径。
/// @param path 输出文件路径。
/// @param size 图像边长（正方形）。
/// @param color 填充色。
/// @return 保存成功返回 true。
static bool writeSolidPng(const QString& path, int size, const QColor& color)
{
    QImage img(size, size, QImage::Format_ARGB32);
    img.fill(color);
    return img.save(path, "PNG");
}

/// @brief 测试期 Qt 告警捕获器（RAII）：安装自定义 message handler 记录
/// QtWarningMsg 及以上级别文本，析构时恢复原 handler。
/// 用于断言「加载失败 qWarning 一次」「绑定缺失零告警」等日志语义。
class WarningCapture {
public:
    /// @brief 安装捕获 handler。
    /// @param out 接收捕获文本的列表（调用方持有，生命周期须覆盖本对象）。
    explicit WarningCapture(QStringList* out) : m_prev(qInstallMessageHandler(handler))
    {
        s_out = out;
    }
    ~WarningCapture() { qInstallMessageHandler(m_prev); s_out = nullptr; }

    WarningCapture(const WarningCapture&) = delete;
    WarningCapture& operator=(const WarningCapture&) = delete;

private:
    static void handler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
    {
        if (type >= QtWarningMsg && s_out)
            s_out->append(msg);
    }
    static QStringList* s_out;               ///< 当前捕获目标（单线程测试假设）。
    QtMessageHandler m_prev;                     ///< 被替换的原 handler，析构时恢复。
};

QStringList* WarningCapture::s_out = nullptr;

class TestImageElement : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    /// @brief 用例1：字面量 src 渲染——临时目录内 8×8 纯红 PNG，
    /// <image src> 渲染后内容区中心像素为纯红。
    void literalSrcRenders()
    {
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), "临时目录创建失败");
        const QString path = dir.filePath(QStringLiteral("red.png"));
        QVERIFY2(writeSolidPng(path, 8, QColor(255, 0, 0)), "红色 PNG 生成失败");

        auto frame = frameFromXmlString(QStringLiteral("<image src=\"%1\"/>").arg(path));
        QVERIFY2(frame, "Frame::fromFile 失败");
        const QImage img = frame->toImage();
        QVERIFY2(!img.isNull(), "渲染结果为空");
        QCOMPARE(img.size(), QSize(8, 8));
        QCOMPARE(img.pixelColor(4, 4), QColor(255, 0, 0));
    }

    /// @brief 用例2：b:src 绑定切换——红/蓝两图，setDynamicProperty 触发
    /// 重物化与重渲染，中心像素由红变蓝。
    void boundSrcSwitches()
    {
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), "临时目录创建失败");
        const QString redPath = dir.filePath(QStringLiteral("red.png"));
        const QString bluePath = dir.filePath(QStringLiteral("blue.png"));
        QVERIFY2(writeSolidPng(redPath, 8, QColor(255, 0, 0)), "红色 PNG 生成失败");
        QVERIFY2(writeSolidPng(bluePath, 8, QColor(0, 0, 255)), "蓝色 PNG 生成失败");

        auto frame = frameFromXmlString(QStringLiteral("<image b:src=\"icon\"/>"));
        QVERIFY2(frame, "Frame::fromFile 失败");

        // 未注入 icon：pixmap 为空 → measure 0×0 → toImage 空图
        QVERIFY2(frame->toImage().isNull(), "未注入 icon 时应为空白（0×0）");

        frame->setDynamicProperty(QStringLiteral("icon"), redPath);
        frame->flush();  // 默认 Coalesced 策略下立即执行待定重布局
        const QImage redImg = frame->toImage();
        QVERIFY2(!redImg.isNull(), "注入红色路径后渲染为空");
        QCOMPARE(redImg.pixelColor(4, 4), QColor(255, 0, 0));

        frame->setDynamicProperty(QStringLiteral("icon"), bluePath);
        frame->flush();
        const QImage blueImg = frame->toImage();
        QVERIFY2(!blueImg.isNull(), "切换到蓝色路径后渲染为空");
        QCOMPARE(blueImg.pixelColor(4, 4), QColor(0, 0, 255));
    }

    /// @brief 用例3：加载失败告警 + 盒模型占位——不存在路径触发一次含路径的
    /// qWarning；measure 内容 0×0 但 margin=5 装饰仍占 10×10。
    void loadFailureWarnsAndBlank()
    {
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), "临时目录创建失败");
        const QString badPath = dir.filePath(QStringLiteral("nonexistent.png"));

        QStringList warnings;
        std::unique_ptr<Node> node;
        {
            const WarningCapture cap(&warnings);
            auto root = parse(QStringLiteral("<image src=\"%1\" margin=\"5\"/>").arg(badPath));
            QVERIFY2(root, "解析失败");
            MapPropertyContext map;
            node = materializeFirst(root, map);
            QVERIFY2(node, "物化失败");

            LayoutContext ctx;
            ctx.ctx = &map;
            LayoutConstraints constraints;
            const MeasureResult result = node->element->measure(ctx, constraints, *node);
            QCOMPARE(result.intrinsicSize, QSizeF(10.0, 10.0));  // 内容 0×0 + margin 5×2
        }

        QCOMPARE(warnings.size(), 1);
        QVERIFY2(warnings.first().contains(QStringLiteral("failed to load image")),
                 qPrintable(QStringLiteral("告警文本不符合预期：%1").arg(warnings.first())));
        QVERIFY2(warnings.first().contains(badPath),
                 "qWarning 应包含失败路径");
        const auto& imageNode = static_cast<const ImageNode&>(*node);
        QVERIFY2(imageNode.pixmap.isNull(), "加载失败时 pixmap 应为空");
    }

    /// @brief 用例4：b:src 属性未注入——零告警、采样点透明（静默回退）。
    void boundSrcMissingSilent()
    {
        QStringList warnings;
        QImage img;
        {
            const WarningCapture cap(&warnings);
            // 显式尺寸保证有可见渲染区域，采样透明而非依赖 0×0 空图
            auto frame = frameFromXmlString(QStringLiteral("<image b:src=\"icon\" width=\"8\" height=\"8\"/>"));
            QVERIFY2(frame, "Frame::fromFile 失败");
            img = frame->toImage();
        }
        QCOMPARE(warnings.size(), 0);
        QVERIFY2(!img.isNull(), "显式尺寸下渲染不应为空");
        QCOMPARE(img.pixelColor(4, 4).alpha(), 0);
    }

    /// @brief 用例5：src="" 空串——零告警、静默空白。
    void emptySrcSilent()
    {
        QStringList warnings;
        QImage img;
        {
            const WarningCapture cap(&warnings);
            auto frame = frameFromXmlString(QStringLiteral("<image src=\"\" width=\"8\" height=\"8\"/>"));
            QVERIFY2(frame, "Frame::fromFile 失败");
            img = frame->toImage();
        }
        QCOMPARE(warnings.size(), 0);
        QVERIFY2(!img.isNull(), "显式尺寸下渲染不应为空");
        QCOMPARE(img.pixelColor(4, 4).alpha(), 0);
    }

    /// @brief 用例6：keep-aspect 默认 true——10×10 绿图放入 20×40 内容区，
    /// 等比适配为 20×20 并垂直居中（y 偏移 10）：(15,20) 命中绿、(2,2) 空白。
    void containCenters()
    {
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), "临时目录创建失败");
        const QString path = dir.filePath(QStringLiteral("green.png"));
        QVERIFY2(writeSolidPng(path, 10, QColor(0, 255, 0)), "绿色 PNG 生成失败");

        auto frame = frameFromXmlString(
            QStringLiteral("<image src=\"%1\" width=\"20\" height=\"40\"/>").arg(path));
        QVERIFY2(frame, "Frame::fromFile 失败");
        const QImage img = frame->toImage();
        QVERIFY2(!img.isNull(), "渲染结果为空");
        QCOMPARE(img.size(), QSize(20, 40));
        QCOMPARE(img.pixelColor(15, 20), QColor(0, 255, 0));  // 居中后的图像区内
        QCOMPARE(img.pixelColor(2, 2).alpha(), 0);            // 顶部留白区
    }

    /// @brief 用例7：keep-aspect="false"——同输入拉伸填满内容区，(2,2) 已填充。
    void stretchWhenNotKeepAspect()
    {
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), "临时目录创建失败");
        const QString path = dir.filePath(QStringLiteral("green.png"));
        QVERIFY2(writeSolidPng(path, 10, QColor(0, 255, 0)), "绿色 PNG 生成失败");

        auto frame = frameFromXmlString(
            QStringLiteral("<image src=\"%1\" width=\"20\" height=\"40\" keep-aspect=\"false\"/>")
                .arg(path));
        QVERIFY2(frame, "Frame::fromFile 失败");
        const QImage img = frame->toImage();
        QVERIFY2(!img.isNull(), "渲染结果为空");
        QCOMPARE(img.pixelColor(2, 2), QColor(0, 255, 0));
        QCOMPARE(img.pixelColor(18, 38), QColor(0, 255, 0));
    }

    /// @brief 用例8：单尺寸推导——10×10 图仅指定 width=20，
    /// measure 内容高度按宽高比推导为 20。
    void singleWidthDerivesHeight()
    {
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), "临时目录创建失败");
        const QString path = dir.filePath(QStringLiteral("square.png"));
        QVERIFY2(writeSolidPng(path, 10, QColor(255, 0, 0)), "PNG 生成失败");

        auto root = parse(QStringLiteral("<image src=\"%1\" width=\"20\"/>").arg(path));
        QVERIFY2(root, "解析失败");
        MapPropertyContext map;
        auto node = materializeFirst(root, map);
        QVERIFY2(node, "物化失败");

        LayoutContext ctx;
        ctx.ctx = &map;
        LayoutConstraints constraints;
        const MeasureResult result = node->element->measure(ctx, constraints, *node);
        QCOMPARE(result.intrinsicSize, QSizeF(20.0, 20.0));
    }

    /// @brief 用例9：模板级缓存命中——同模板两次物化同路径，
    /// 两次节点共享同一 QPixmap 缓存项（cacheKey 相等，无侵入证明未重复加载）。
    void cacheHitSkipsReload()
    {
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), "临时目录创建失败");
        const QString path = dir.filePath(QStringLiteral("cached.png"));
        QVERIFY2(writeSolidPng(path, 8, QColor(255, 0, 0)), "PNG 生成失败");

        auto root = parse(QStringLiteral("<image src=\"%1\"/>").arg(path));
        QVERIFY2(root, "解析失败");
        MapPropertyContext map;
        auto first = materializeFirst(root, map);
        auto second = materializeFirst(root, map);
        QVERIFY2(first && second, "两次物化均应成功");

        const auto& firstImg = static_cast<const ImageNode&>(*first);
        const auto& secondImg = static_cast<const ImageNode&>(*second);
        QVERIFY2(!firstImg.pixmap.isNull() && !secondImg.pixmap.isNull(),
                 "两次物化均应加载到图像");
        QCOMPARE(secondImg.pixmap.cacheKey(), firstImg.pixmap.cacheKey());
    }

    /// @brief 用例10：失败路径入集——同一坏路径两次物化，qWarning 仅一次。
    void failureCachedWarnsOnce()
    {
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), "临时目录创建失败");
        const QString badPath = dir.filePath(QStringLiteral("missing.png"));

        auto root = parse(QStringLiteral("<image src=\"%1\"/>").arg(badPath));
        QVERIFY2(root, "解析失败");
        MapPropertyContext map;

        QStringList warnings;
        {
            const WarningCapture cap(&warnings);
            auto first = materializeFirst(root, map);
            auto second = materializeFirst(root, map);
            QVERIFY2(first && second, "两次物化均应成功（失败静默空白）");
        }
        QCOMPARE(warnings.size(), 1);
        QVERIFY2(warnings.first().contains(badPath),
                 "唯一一次告警应包含失败路径");
    }

    /// @brief 用例11：qrc 内嵌资源加载——:/icons/red.png 采样命中红色，
    /// 实证 qrc 测试基建（icons.qrc 经 qt5_add_resources 编入本目标）。
    /// 注：本 Qt5 构建上 "qrc:/" 方案前缀不可用（QPixmap 走文件加载并 qWarning、
    /// QFile::exists 亦判不存在），资源路径一律用 ":/" 前缀形式。
    void qrcPathLoads()
    {
        // 资源是否注册：":/" 前缀探测资源系统可见性
        QVERIFY2(QFile::exists(QStringLiteral(":/icons/red.png")),
                 "qrc 资源未注册进测试二进制");
        auto frame = frameFromXmlString(QStringLiteral("<image src=\":/icons/red.png\"/>"));
        QVERIFY2(frame, "Frame::fromFile 失败");
        const QImage img = frame->toImage();
        QVERIFY2(!img.isNull(), "qrc 图像渲染为空");
        QCOMPARE(img.pixelColor(4, 4), QColor(255, 0, 0));
    }

    /// @brief 用例12：相对路径按 cwd 解析——受控 QDir::setCurrent(临时目录) 后
    /// 相对 src 加载成功；用例末尾恢复 cwd（保持该用例最后注册）。
    void relativePathResolvesFromCwd()
    {
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), "临时目录创建失败");
        QVERIFY2(writeSolidPng(dir.filePath(QStringLiteral("rel.png")), 8,
                               QColor(255, 0, 0)),
                 "PNG 生成失败");

        const QString oldCwd = QDir::currentPath();
        QVERIFY2(QDir::setCurrent(dir.path()), "切换 cwd 到临时目录失败");

        auto root = parse(QStringLiteral("<image src=\"rel.png\"/>"));
        QVERIFY2(root, "解析失败");
        MapPropertyContext map;
        auto node = materializeFirst(root, map);
        QVERIFY2(node, "相对路径物化失败");
        const auto& imageNode = static_cast<const ImageNode&>(*node);
        QVERIFY2(!imageNode.pixmap.isNull(), "相对路径应成功加载图像");
        QCOMPARE(imageNode.pixmap.width(), 8);

        QVERIFY2(QDir::setCurrent(oldCwd), "恢复 cwd 失败");
    }
    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestImageElement)
#include "test_image_element.moc"
