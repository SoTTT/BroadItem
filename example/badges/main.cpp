#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPainter>
#include <QStringList>
#include <QDebug>
#include <broaditem/core/BroadItem.h>

#include "../common/scene_shot.h"

/// @brief 入口点。集中展示 example/badges/ 画廊标牌，纵向堆叠于同一场景。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;

    const QString dir = QApplication::applicationDirPath();
    const QStringList layouts = {
        QStringLiteral("breaker.xml"),
        QStringLiteral("telemetry.xml"),
        QStringLiteral("alarm_banner.xml"),
        QStringLiteral("switch_ports.xml"),
        QStringLiteral("pump.xml"),
    };

    // 依次加载画廊布局，flush 后按实际高度纵向堆叠。
    qreal y = 0.0;
    for (const QString &name : layouts) {
        auto* item = new BroadItem::BroadItem(dir + QLatin1Char('/') + name);
        item->flush();
        // 加载失败（布局为空）时告警并跳过该布局，避免把空 item 叠进场景
        if (item->boundingRect().isEmpty()) {
            qWarning() << "布局加载失败，跳过:" << name;
            delete item;
            continue;
        }
        item->setPos(0.0, y);
        scene.addItem(item);
        y += item->boundingRect().height() + 12.0;
    }
    scene.setSceneRect(scene.itemsBoundingRect().adjusted(-12.0, -12.0, 12.0, 12.0));

    // 可选：--shot <path> 将场景渲染为 PNG 后退出（离屏核验 / 配图更新用）。
    // 与共享头 handleSmokeShotArgs 的定时器延迟版不同：此处 item 均已 flush，
    // 场景在进事件循环前即为终态，同步出图并直接以保存结果作退出码。
    const QStringList args = QApplication::arguments();
    const int shotIdx = args.indexOf(QStringLiteral("--shot"));
    if (shotIdx >= 0 && shotIdx + 1 < args.size())
        return renderSceneToPng(scene, args.at(shotIdx + 1)) ? 0 : 1;

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle(QStringLiteral("BroadItem 标牌画廊"));
    view.resize(400, static_cast<int>(scene.sceneRect().height()) + 24);
    view.show();

    return QApplication::exec();
}
