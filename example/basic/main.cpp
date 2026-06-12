#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <broaditem/core/BroadItem.h>
#include <broaditem/parser/LayoutRegistry.h>
#include <QDebug>

/// @brief 入口点。加载 XML 布局并在 QGraphicsView 中显示。
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 400, 300);

    auto *item = new BroadItem::BroadItem(QApplication::applicationDirPath() + "/basic.xml");
    item->setDynamicProperty("title", "设备状态");
    item->setDynamicProperty("status", "运行中");
    item->setDynamicProperty("ip", "192.168.1.100");
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle("BroadItem Test");
    view.resize(420, 320);
    view.show();

    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    return app.exec();
}
