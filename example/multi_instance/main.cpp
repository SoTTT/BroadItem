/**
 * @file main.cpp
 * @brief BroadItem 多实例隔离可视化示例。
 *
 * 同一 layoutId 注册布局实例化两个状态卡，另有一种进程列表布局实例，
 * 三个实例共用模板层（Element 树不可变、Registry 共享），实例层各自持有
 * 独立 Node 树与默认 PropertyContext，配合各自独立的 QTimer 以不同间隔
 * 刷新互不相同的数据，肉眼可辨地验证模板/实例分离重构的隔离效果。
 * 所有实例均可拖动（QGraphicsItem::ItemIsMovable）。
 *
 * 用法：
 *   multi_instance                正常运行，进入事件循环。
 *   multi_instance --smoke        冒烟模式：约 3 秒后自动退出（退出码 0），
 *                                 用于 offscreen 环境下验证构造与多轮刷新不崩溃。
 *   multi_instance --shot <path>  截图模式：约 2.6 秒后（500ms 定时器已触发约 5 轮、
 *                                 900ms 约 2 轮、1500ms 1 轮，三卡数据已各自刷新出差异）
 *                                 将整个场景经 scene.render() 离屏渲染为 PNG 落盘并退出，
 *                                 用于 offscreen 视觉验证。与 --smoke 可独立使用。
 */

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSimpleTextItem>
#include <QTimer>
#include <QRandomGenerator>
#include <QStringList>
#include <QImage>
#include <QPainter>
#include <QDebug>

#include <broaditem/core/BroadItem.h>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/parser/LayoutRegistry.h>

#include "../common/scene_shot.h"

namespace {

/// @brief 状态卡布局注册 ID（两个实例共用同一 ID，隔离验证的关键）。
constexpr int kStatusCardLayoutId = 1001;
/// @brief 进程列表布局注册 ID。
constexpr int kProcessListLayoutId = 1002;

/// @brief 返回 [0, max) 范围内的随机整数。
/// @param max 上界（不含）。
/// @return 随机整数。
int rnd(int max)
{
    return QRandomGenerator::global()->bounded(max);
}

/// @brief 生成随机百分比字符串（例如 "45%"）。
/// @param base 基准值。
/// @param span 随机跨度。
/// @return 百分比字符串。
QString randomPercent(int base, int span)
{
    return QString::number(base + rnd(span)) + QStringLiteral("%");
}

/// @brief 轮转返回状态文本，让每次刷新的变化肉眼可辨。
/// @return "在线" / "繁忙" / "空闲" 之一。
QString nextStatus()
{
    static const QStringList pool = {
        QStringLiteral("在线"),
        QStringLiteral("繁忙"),
        QStringLiteral("空闲")
    };
    static int index = 0;
    return pool[(index++) % pool.size()];
}

/// @brief 每次调用重新生成进程列表：从名称池轮转窗口抽取进程并附随机 CPU。
///
/// 名称池固定 8 项，每次以步进 1 的轮转起点截取 3~5 项，
/// 使 for 展平路径在每次刷新时都被不同长度、不同内容的列表持续验证。
///
/// @return 进程列表，元素为 {name, cpu} 的 QVariantMap。
QVariantList makeProcesses()
{
    static const QStringList namePool = {
        QStringLiteral("nginx"),
        QStringLiteral("postgres"),
        QStringLiteral("redis-server"),
        QStringLiteral("node"),
        QStringLiteral("dockerd"),
        QStringLiteral("kernel_task"),
        QStringLiteral("WindowServer"),
        QStringLiteral("sshd")
    };
    static int start = 0;

    const int count = 3 + rnd(3);  // 3~5 行，列表长度随刷新变化
    QVariantList list;
    for (int i = 0; i < count; ++i) {
        QVariantMap p;
        p[QStringLiteral("name")] = namePool[(start + i) % namePool.size()];
        p[QStringLiteral("cpu")] =
            QString::number(rnd(1000) / 10.0, 'f', 1) + QStringLiteral(" %");
        list.append(p);
    }
    start = (start + 1) % namePool.size();
    return list;
}

/// @brief 以注册布局 ID 创建一个可拖动的 BroadItem 并加入场景。
/// @param layoutId 已注册的布局 ID。
/// @param pos 初始场景位置。
/// @param scene 目标场景。
/// @return 新建的 BroadItem 指针（由场景接管生命周期）。
BroadItem::BroadItem* makeItem(int layoutId, const QPointF& pos, QGraphicsScene* scene)
{
    // 走 Registry 共享模板路径（默认构造 PropertyContext，各实例互不共享）。
    auto* item = new BroadItem::BroadItem(layoutId);
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setPos(pos);
    scene->addItem(item);
    return item;
}

/// @brief 刷新状态卡实例的数据（节点名固定，状态轮转，CPU/内存随机）。
/// @param item 目标状态卡实例。
/// @param nodeName 节点名称（区分实例身份，如 "节点 A" / "节点 B"）。
void refreshStatusCard(BroadItem::BroadItem* item, const QString& nodeName)
{
    item->setDynamicProperty(QStringLiteral("title"), nodeName);
    item->setDynamicProperty(QStringLiteral("status"), nextStatus());
    item->setDynamicProperty(QStringLiteral("cpu"), randomPercent(10, 85));
    item->setDynamicProperty(QStringLiteral("memory"), randomPercent(20, 75));
}

/// @brief 刷新进程列表实例的数据（每次重新生成列表）。
/// @param item 目标进程列表实例。
void refreshProcessList(BroadItem::BroadItem* item)
{
    item->setDynamicProperty(QStringLiteral("title"), QStringLiteral("活跃进程"));
    item->setDynamicProperty(QStringLiteral("processes"), makeProcesses());
}

} // namespace

/// @brief 入口点。注册两种布局，创建三个可拖动实例并各自独立定时刷新。
/// @param argc 参数个数。
/// @param argv 参数列表（支持 --smoke 冒烟模式）。
/// @return 退出码（正常退出为 0；布局解析失败为 1）。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 解析两种布局并注册：模板层 Element 树由 Registry 共享，实例层各自独立。
    const QString dir = QApplication::applicationDirPath();
    auto statusCardRoot = BroadItem::XmlLayoutParser::parseFile(dir + QStringLiteral("/status_card.xml"));
    auto processListRoot = BroadItem::XmlLayoutParser::parseFile(dir + QStringLiteral("/process_list.xml"));
    if (!statusCardRoot || !processListRoot) {
        qWarning() << "布局 XML 解析失败，请确认已重新构建（configure_file 拷贝）";
        return 1;
    }
    BroadItem::LayoutRegistry::instance().registerLayout(kStatusCardLayoutId, statusCardRoot);
    BroadItem::LayoutRegistry::instance().registerLayout(kProcessListLayoutId, processListRoot);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 800, 400);
    scene.setBackgroundBrush(QColor(QStringLiteral("#11111b")));

    // 三个实例：状态卡 ×2（同一 layoutId，隔离验证核心）+ 进程列表 ×1，横向排开。
    auto* cardA = makeItem(kStatusCardLayoutId, QPointF(20, 20), &scene);
    auto* cardB = makeItem(kStatusCardLayoutId, QPointF(280, 20), &scene);
    auto* procList = makeItem(kProcessListLayoutId, QPointF(560, 20), &scene);

    // 场景说明文字。
    auto* hint = scene.addSimpleText(QStringLiteral("拖动卡片 · 同布局双实例 · 各自独立刷新"));
    QFont hintFont = hint->font();
    hintFont.setPointSize(13);
    hint->setFont(hintFont);
    hint->setBrush(QColor(QStringLiteral("#6c7086")));
    hint->setPos(20, 356);

    // 初始数据。
    refreshStatusCard(cardA, QStringLiteral("节点 A"));
    refreshStatusCard(cardB, QStringLiteral("节点 B"));
    refreshProcessList(procList);

    // 独立刷新定时器：500ms / 900ms / 1500ms 不同间隔，数据内容互不相同。
    auto* timerA = new QTimer(&app);
    QObject::connect(timerA, &QTimer::timeout, &app, [cardA]() {
        refreshStatusCard(cardA, QStringLiteral("节点 A"));
    });
    timerA->start(500);

    auto* timerB = new QTimer(&app);
    QObject::connect(timerB, &QTimer::timeout, &app, [cardB]() {
        refreshStatusCard(cardB, QStringLiteral("节点 B"));
    });
    timerB->start(900);

    auto* timerP = new QTimer(&app);
    QObject::connect(timerP, &QTimer::timeout, &app, [procList]() {
        refreshProcessList(procList);
    });
    timerP->start(1500);

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle(QStringLiteral("BroadItem 多实例隔离示例"));
    view.resize(820, 420);
    view.show();

    // 冒烟模式：--smoke 时约 3 秒后自动退出（退出码 0），
    // 验证三个实例的构造与多轮独立刷新不崩溃；默认正常进入事件循环。
    // 截图模式：--shot <path> 时约 2.6 秒后将整个场景离屏渲染为 PNG 落盘并退出。
    // 此刻 500ms 定时器已触发约 5 轮、900ms 约 2 轮、1500ms 1 轮，
    // 三张卡数据已各自刷新出肉眼可辨差异，适合 offscreen 视觉验证。
    handleSmokeShotArgs(app, scene);

    return QApplication::exec();
}
