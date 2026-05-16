#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include "broaditem/BroadItem.h"
#include "broaditem/MapPropertyContext.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 获取 XML 文件路径（与可执行文件同级目录）
    QString xmlPath = QFileInfo(QString::fromUtf8(argv[0])).path() + "/layout.xml";
    
    // 1. 创建数据上下文
    auto propCtx = std::make_shared<BroadItem::MapPropertyContext>();
    propCtx->setProperty("title", "系统状态监控");
    propCtx->setProperty("cpu", "45%");
    propCtx->setProperty("memory", "2.3GB / 8GB");

    // 2. 从文件构造 BroadItem
    auto* item = new BroadItem::BroadItem(xmlPath, propCtx);
    if (item->boundingRect().isEmpty()) {
        qCritical() << "Failed to load layout from:" << xmlPath;
        return 1;
    }

    // 3. 创建场景并添加 item
    QGraphicsScene scene;
    scene.addItem(item);
    
    // 调整场景大小以匹配 item
    scene.setSceneRect(item->boundingRect().adjusted(-20, -20, 20, 20));

    // 4. 创建视图
    QGraphicsView view(&scene);
    view.setWindowTitle("BroadItem Playground");
    view.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    view.setBackgroundBrush(QBrush(QColor(0xECEFF1)));
    view.setFixedSize(400, 300);
    view.setAlignment(Qt::AlignCenter);
    
    // 显示
    view.show();

    return QApplication::exec();
}
