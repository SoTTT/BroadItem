#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <broaditem/core/BroadItem.h>
#include <QTimer>
#include <QRandomGenerator>

/// @brief 返回 [0, max) 范围内的随机整数。
static int rnd(int max)
{
    return QRandomGenerator::global()->bounded(max);
}

/// @brief 生成随机 CPU 使用率字符串（例如 "45%"）。
static QString randomCpu()
{
    return QString::number(20 + rnd(60)) + "%";
}

/// @brief 生成随机内存使用率字符串（例如 "65%"）。
static QString randomMemory()
{
    return QString::number(30 + rnd(50)) + "%";
}

/// @brief 生成随机磁盘使用率字符串（例如 "72%"）。
static QString randomDisk()
{
    return QString::number(40 + rnd(50)) + "%";
}

/// @brief 生成随机网络入站速度字符串（MB/s）。
static QString randomNetIn()
{
    return QString::number(rnd(200) / 10.0, 'f', 1) + " MB/s";
}

/// @brief 生成随机网络出站速度字符串（MB/s）。
static QString randomNetOut()
{
    return QString::number(rnd(80) / 10.0, 'f', 1) + " MB/s";
}

/// @brief 生成随机温度字符串（例如 "63°C"）。
static QString randomTemp()
{
    return QString::number(45 + rnd(35)) + "°C";
}

/// @brief 构建模拟运行进程列表（名称、PID、CPU 百分比）。
static QVariantList makeProcesses()
{
    QVariantList list;
    {
        QVariantMap p;
        p["name"] = "com.apple.WebKit.WebContent";
        p["pid"] = 87471;
        p["cpu"] = 12.3;
        list.append(p);
    }
    {
        QVariantMap p;
        p["name"] = "kernel_task";
        p["pid"] = 0;
        p["cpu"] = 4.5;
        list.append(p);
    }
    {
        QVariantMap p;
        p["name"] = "WindowServer";
        p["pid"] = 199;
        p["cpu"] = 3.1;
        list.append(p);
    }
    return list;
}

/// @brief 入口点。加载复杂的服务器监控布局并通过定时器动画显示实时指标。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 500, 500);

    auto* item = new BroadItem::BroadItem(QApplication::applicationDirPath() + "/complex.xml");

    // 初始数据
    item->setDynamicProperty("device_name", "服务器 #01");
    item->setDynamicProperty("status_text", "运行中");
    item->setDynamicProperty("cpu", randomCpu());
    item->setDynamicProperty("memory", randomMemory());
    item->setDynamicProperty("disk", randomDisk());
    item->setDynamicProperty("net_in", randomNetIn());
    item->setDynamicProperty("net_out", randomNetOut());
    item->setDynamicProperty("temp", randomTemp());

    item->setDynamicProperty("ipv4", "192.168.1.100");
    item->setDynamicProperty("mac", "AA:BB:CC:DD:EE:FF");
    item->setDynamicProperty("gateway", "192.168.1.1");
    item->setDynamicProperty("dns", "8.8.8.8");

    item->setDynamicProperty("warning", "CPU 使用率超过阈值！");
    item->setDynamicProperty("processes", makeProcesses());

    scene.addItem(item);

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle("BroadItem Complex Test");
    view.resize(520, 520);
    view.show();

    // 定时更新数据
    auto* timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, [item]() {
        item->setDynamicProperty("cpu", randomCpu());
        item->setDynamicProperty("memory", randomMemory());
        item->setDynamicProperty("disk", randomDisk());
        item->setDynamicProperty("net_in", randomNetIn());
        item->setDynamicProperty("net_out", randomNetOut());
        item->setDynamicProperty("temp", randomTemp());
        item->setDynamicProperty("processes", makeProcesses());

        // 偶尔移除/恢复警告
        if (rnd(5) == 0) {
            item->setDynamicProperty("warning", QVariant{});
        } else {
            static const QStringList warnings = {
                "CPU 使用率超过阈值！",
                "内存占用接近上限",
                "磁盘空间不足",
                "网络延迟异常"
            };
            item->setDynamicProperty("warning", warnings[rnd(warnings.size())]);
        }
    });
    timer->start(1500);

    return QApplication::exec();
}
