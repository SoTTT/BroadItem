#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <broaditem/core/BroadItem.h>

/// @brief 创建一个用于标注的说明文字矩形（不是 BroadItem 的一部分，仅用于视觉分隔）。
static QGraphicsRectItem* makeLabelRect(const QString& text, double x, double y, double w, double h)
{
    auto* rect = new QGraphicsRectItem(x, y, w, h);
    rect->setBrush(QBrush(QColor("#E3F2FD")));
    rect->setPen(QPen(QColor("#2196F3")));

    auto* label = new QGraphicsTextItem(text, rect);
    label->setDefaultTextColor(Qt::black);
    label->setPos(x + 8, y + 4);

    return rect;
}

/// @brief 入口点。加载两个对比布局：左侧未启用 main-stretch，右侧启用。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 700, 420);

    const QString basePath = QApplication::applicationDirPath();

    // 上方：未启用 main-stretch 的行，文本标签宽度由各自内容决定
    auto* withoutItem = new BroadItem::BroadItem(basePath + "/main_stretch_without.xml");
    withoutItem->setPos(40, 80);
    scene.addItem(withoutItem);
    scene.addItem(makeLabelRect("未启用 main-stretch：子元素宽度由内容决定", 30, 40, 640, 32));

    // 下方：启用 main-stretch 的行，文本标签按最宽者统一
    auto* withItem = new BroadItem::BroadItem(basePath + "/main_stretch_with.xml");
    withItem->setPos(40, 260);
    scene.addItem(withItem);
    scene.addItem(makeLabelRect("启用 main-stretch：子元素宽度统一", 30, 220, 640, 32));

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle("BroadItem main-stretch 对比示例");
    view.resize(720, 480);
    view.show();

    return QApplication::exec();
}
