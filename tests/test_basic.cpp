#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include "broaditem/BroadItem.h"
#include "broaditem/LayoutRegistry.h"
#include <QDebug>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 400, 300);

    BroadItem::BroadItem* item = new BroadItem::BroadItem("../tests/test_layout.xml");
    item->setDynamicProperty("title", "设备状态");
    item->setDynamicProperty("status", "运行中");
    item->setDynamicProperty("ip", "192.168.1.100");
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle("BroadItem Test");
    view.resize(420, 320);
    view.show();

    return app.exec();
}
