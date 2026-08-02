/**
 * @file main.cpp
 * @brief BroadItem 监控铭牌示例：集中展示 P0-1（通用绑定）与 P0-2（<image> 元素）。
 *
 * 场景为一台 BroadItem（从 XML 文件直接构造，无需注册 layoutId），
 * 内含 <for> 展开的三台设备卡片。每张卡片的图标（b:src）、状态文字
 * （b:content）、文字颜色（b:color）与背景色（b:background-color）全部
 * 由数据绑定驱动；QTimer 每 800ms 把一台设备轮换到下一状态
 * （ok→warn→error 循环），setDynamicProperty("devices", ...) 命中
 * <for> 的 b:of 前缀匹配后触发整列重新物化，演示完整的绑定传播刷新链。
 *
 * 三个状态图标（8×8 彩色圆点 PNG）由代码生成到当前工作目录下的
 * status_icons/ 子目录（已存在则跳过），零二进制素材入库；
 * XML 中的图标路径（status_icons/ok.png 等）相对 cwd 解析，
 * 示例自身即演示 b:src 的"相对工作目录"语义。
 *
 * 用法：
 *   status_panel                正常运行，进入事件循环。
 *   status_panel --smoke        冒烟模式：约 3 秒后自动退出（退出码 0），
 *                               用于 offscreen 环境下验证构造与多轮刷新不崩溃。
 *   status_panel --shot <path>  截图模式：约 2.6 秒后（800ms 定时器已触发 3 轮，
 *                               三台设备已各轮换一次、状态错开）将场景经
 *                               scene.render() 离屏渲染为 PNG 落盘并退出，
 *                               用于 offscreen 视觉验证。
 */

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QVector>
#include <QDebug>

#include <broaditem/core/BroadItem.h>

#include "../common/scene_shot.h"

namespace {

/// @brief 状态总数（ok / warn / error 循环）。
constexpr int kStateCount = 3;

/// @brief 状态图标文件名（不含目录）。
const char* kIconNames[kStateCount] = { "ok.png", "warn.png", "error.png" };

/// @brief 状态图标填充色。
const char* kIconColors[kStateCount] = { "#4caf50", "#ffc107", "#f44336" };

/// @brief 状态文字（ASCII：offscreen 环境无 CJK 字体，保证 --shot 截图可读）。
const char* kStatusTexts[kStateCount] = { "OK", "WARN", "ERROR" };

/// @brief 状态文字颜色。
const char* kStatusColors[kStateCount] = { "#2e7d32", "#ff8f00", "#c62828" };

/// @brief 卡片背景色（浅色系，与状态色同族）。
const char* kBgColors[kStateCount] = { "#e8f5e9", "#fff8e1", "#ffebee" };

/// @brief 首次启动时生成三个状态图标（8×8 彩色圆点 PNG）到 cwd/status_icons/。
///
/// 已存在的文件直接跳过，保证零二进制素材入库且重复运行无副作用。
///
/// @return 图标目录的相对路径前缀（"status_icons/"）。
QString ensureStatusIcons()
{
    const QString dirName = QStringLiteral("status_icons");
    QDir dir(QDir::currentPath());
    if (!dir.exists(dirName))
        dir.mkpath(dirName);

    for (int i = 0; i < kStateCount; ++i) {
        const QString path = dirName + QLatin1Char('/') + QLatin1String(kIconNames[i]);
        if (QFile::exists(path))
            continue;

        QImage image(8, 8, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(QLatin1String(kIconColors[i])));
        painter.drawEllipse(image.rect());
        painter.end();
        if (!image.save(path))
            qWarning() << "图标生成失败:" << path;
    }
    return dirName + QLatin1Char('/');
}

/// @brief 按设备索引与状态索引组装一台设备的数据 map。
/// @param deviceIndex 设备索引（0..2）。
/// @param state 状态索引（0=ok, 1=warn, 2=error）。
/// @param iconPrefix 图标路径前缀（"status_icons/"）。
/// @return 设备数据 QVariantMap（name/icon/status/statusColor/bgColor）。
QVariantMap makeDevice(int deviceIndex, int state, const QString& iconPrefix)
{
    static const QStringList names = {
        QStringLiteral("Switch A"),
        QStringLiteral("Server B"),
        QStringLiteral("Sensor C")
    };
    QVariantMap d;
    d[QStringLiteral("name")] = names.at(deviceIndex);
    d[QStringLiteral("icon")] = iconPrefix + QLatin1String(kIconNames[state]);
    d[QStringLiteral("status")] = QLatin1String(kStatusTexts[state]);
    d[QStringLiteral("statusColor")] = QLatin1String(kStatusColors[state]);
    d[QStringLiteral("bgColor")] = QLatin1String(kBgColors[state]);
    return d;
}

} // namespace

/// @brief 入口点。生成图标、构造监控铭牌并启动 800ms 状态轮换。
/// @param argc 参数个数。
/// @param argv 参数列表（支持 --smoke / --shot <path>）。
/// @return 退出码（正常退出为 0；布局解析失败为 1）。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 零二进制素材：首次运行生成状态图标到 cwd/status_icons/。
    const QString iconPrefix = ensureStatusIcons();

    // 从文件直接构造 BroadItem（layoutId 可选路径的另一种用法）。
    const QString xmlPath =
        QApplication::applicationDirPath() + QStringLiteral("/status_panel.xml");
    auto* item = new BroadItem::BroadItem(xmlPath);
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setPos(20, 20);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 320, 180);
    scene.setBackgroundBrush(QColor(QStringLiteral("#e0e0e0")));
    scene.addItem(item);

    // 设备状态表：states[i] 为第 i 台设备当前状态索引，初始错开便于首帧即有区分度。
    QVector<int> states = { 0, 1, 2 };
    const int deviceCount = states.size();

    // 组装整列数据并一次性下发：<for> 的 b:of 前缀匹配命中后整列重新物化。
    const auto pushDevices = [item, &states, deviceCount, iconPrefix]() {
        QVariantList devices;
        for (int i = 0; i < deviceCount; ++i)
            devices.append(makeDevice(i, states[i], iconPrefix));
        item->setDynamicProperty(QStringLiteral("devices"), devices);
    };
    pushDevices();

    // 每 800ms 把一台设备轮换到下一状态（ok→warn→error），
    // 图标、状态文字、文字颜色、背景色随数据同步联动切换。
    auto* timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, &app, [&states, pushDevices]() {
        static int cursor = 0;
        states[cursor] = (states[cursor] + 1) % kStateCount;
        cursor = (cursor + 1) % states.size();
        pushDevices();
    });
    timer->start(800);

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle(QStringLiteral("BroadItem 监控铭牌示例"));
    view.resize(340, 200);
    view.show();

    // 冒烟模式：约 3 秒后自动退出（800ms 定时器已触发约 3 轮），
    // 验证构造与绑定传播刷新链在 offscreen 下不崩溃。
    // 截图模式：约 2.6 秒后离屏渲染 PNG 落盘并退出，
    // 此刻三台设备已各轮换一次，三卡状态错开、视觉差异成立。
    handleSmokeShotArgs(app, scene);

    return QApplication::exec();
}
