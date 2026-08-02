#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QStringList>
#include <QDebug>
#include <broaditem/core/BroadItem.h>

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
    const QStringList args = QApplication::arguments();
    const int shotIdx = args.indexOf(QStringLiteral("--shot"));
    if (shotIdx >= 0 && shotIdx + 1 < args.size()) {
        QImage image(scene.sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        scene.render(&painter);
        painter.end();
        return image.save(args.at(shotIdx + 1)) ? 0 : 1;
    }

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle(QStringLiteral("BroadItem 标牌画廊"));
    view.resize(400, static_cast<int>(scene.sceneRect().height()) + 24);
    view.show();

    return QApplication::exec();
}
