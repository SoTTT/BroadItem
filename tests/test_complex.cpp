#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include "broaditem/BroadItem.h"
#include <QDebug>
#include <QTimer>
#include <QTime>
#include <cmath>

static QString randomCpu()
{
    return QString::number(20 + (qrand() % 60)) + "%";
}

static QString randomMemory()
{
    return QString::number(30 + (qrand() % 50)) + "%";
}

static QString randomDisk()
{
    return QString::number(40 + (qrand() % 50)) + "%";
}

static QString randomNetIn()
{
    return QString::number((qrand() % 200) / 10.0, 'f', 1) + " MB/s";
}

static QString randomNetOut()
{
    return QString::number((qrand() % 80) / 10.0, 'f', 1) + " MB/s";
}

static QString randomTemp()
{
    return QString::number(45 + (qrand() % 35)) + "°C";
}

static QStringList makeProcesses()
{
    QStringList names = {"nginx", "mysql", "redis", "docker", "chrome", "node", "ssh", "postgres"};
    QStringList procs;
    int count = 3 + (qrand() % 4); // 3~6 个进程
    for (int i = 0; i < count; ++i) {
        procs.append(names[qrand() % names.size()]);
        procs.append(QString::number(1000 + (qrand() % 9000)));
        procs.append(QString::number((qrand() % 150) / 10.0, 'f', 1) + "%");
    }
    return procs;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qsrand(QTime::currentTime().msec());

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 500, 500);

    BroadItem::BroadItem* item = new BroadItem::BroadItem("../tests/test_complex.xml");

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
    QTimer* timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, [item]() {
        item->setDynamicProperty("cpu", randomCpu());
        item->setDynamicProperty("memory", randomMemory());
        item->setDynamicProperty("disk", randomDisk());
        item->setDynamicProperty("net_in", randomNetIn());
        item->setDynamicProperty("net_out", randomNetOut());
        item->setDynamicProperty("temp", randomTemp());
        item->setDynamicProperty("processes", makeProcesses());

        // 偶尔移除/恢复警告
        if (qrand() % 5 == 0) {
            item->setDynamicProperty("warning", "");
        } else {
            static const QStringList warnings = {
                "CPU 使用率超过阈值！",
                "内存占用接近上限",
                "磁盘空间不足",
                "网络延迟异常"
            };
            item->setDynamicProperty("warning", warnings[qrand() % warnings.size()]);
        }
    });
    timer->start(1500);

    return app.exec();
}
