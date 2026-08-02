#include <QApplication>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>

#include <broaditem/reactive/AnchorDecorator.h>
#include <broaditem/reactive/AnchorPoint.h>
#include <broaditem/reactive/ConnectionLine.h>
#include <broaditem/reactive/FollowBinding.h>

#include <QKeyEvent>
#include <QDebug>

/// @brief 示例用的彩色矩形项，直接继承 QGraphicsObject 以支持属性绑定。
class ColoredRect : public QGraphicsObject {
    Q_OBJECT
public:
    /// @brief 构造彩色矩形项。
    /// @param color 填充颜色。
    /// @param width 矩形宽度。
    /// @param height 矩形高度。
    /// @param parent 父项。
    explicit ColoredRect(const QColor& color, qreal width, qreal height,
                         QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent)
        , m_color(color)
        , m_width(width)
        , m_height(height)
    {
        setFlags(flags() | QGraphicsItem::ItemIsMovable
                         | QGraphicsItem::ItemSendsGeometryChanges
                         | QGraphicsItem::ItemSendsScenePositionChanges);
    }

    /// @brief 返回项的包围矩形。
    [[nodiscard]] QRectF boundingRect() const override
    {
        return QRectF(0, 0, m_width, m_height);
    }

    /// @brief 绘制矩形。
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*,
               QWidget*) override
    {
        painter->setBrush(m_color);
        painter->setPen(Qt::black);
        painter->drawRect(boundingRect());
    }

private:
    QColor m_color;  ///< 填充颜色
    qreal m_width;   ///< 矩形宽度
    qreal m_height;  ///< 矩形高度
};

/// @brief 支持键盘交互的视图，处理退出与删除按键。
class InteractiveView : public QGraphicsView {
    Q_OBJECT
public:
    /// @brief 构造视图。
    /// @param scene 关联的场景。
    explicit InteractiveView(QGraphicsScene* scene, QWidget* parent = nullptr)
        : QGraphicsView(scene, parent) {}

signals:
    /// @brief 用户按下 D 键时发出，请求删除黄色矩形。
    void deleteRequested();

    /// @brief 用户按下 B 键时发出，请求删除蓝色矩形。
    void deleteBlueRequested();

protected:
    /// @brief 处理键盘事件。
    /// @param event 键盘事件。
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Q) {
            qApp->quit();
            return;
        }
        if (event->key() == Qt::Key_D) {
            emit deleteRequested();
            return;
        }
        if (event->key() == Qt::Key_B) {
            emit deleteBlueRequested();
            return;
        }
        QGraphicsView::keyPressEvent(event);
    }
};

/// @brief 入口点。演示 AnchorDecorator + AnchorPoint + ConnectionLine + FollowBinding 集成。
///
/// 三个 ColoredRect 各自由 AnchorDecorator 包裹，装饰器提供锚点和外框。
/// ConnectionLine 通过锚点连接装饰器，FollowBinding 在装饰器层级实现位置跟随链：
/// leaderDecorator → followerDecorator → secondFollowerDecorator。
/// 拖拽由装饰器接管：红色矩形的 AnchorDecorator 处理鼠标拖拽（ItemIsMovable），
/// 矩形自身禁用鼠标与移动标志。蓝色和黄色矩形通过 FollowBinding 级联跟随并保持相对偏移。
/// Esc/Q 退出，D 删除黄色矩形（级联销毁其装饰器和锚点），B 删除蓝色矩形。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 600, 400);

    auto* leader = new ColoredRect(Qt::red, 60, 60);
    auto* follower = new ColoredRect(Qt::blue, 60, 60);
    auto* secondFollower = new ColoredRect(Qt::yellow, 60, 60);

    QPointF offset(120.0, 80.0);
    QPointF secondOffset(120.0, 0.0);

    leader->setPos(50, 170);
    follower->setPos(leader->pos() + offset);
    secondFollower->setPos(follower->pos() + secondOffset);

    scene.addItem(leader);
    scene.addItem(follower);
    scene.addItem(secondFollower);

    // 创建 AnchorDecorator 包裹每个矩形，提供锚点和外框。
    // create() 后矩形的 parentItem 变为装饰器，装饰器根据矩形包围盒自定位。
    auto* leaderDecorator =
        BroadItem::AnchorDecorator::create(&scene, leader);
    auto* followerDecorator =
        BroadItem::AnchorDecorator::create(&scene, follower);
    auto* secondFollowerDecorator =
        BroadItem::AnchorDecorator::create(&scene, secondFollower);

    // 设置装饰器外框画笔颜色以区分三个矩形
    leaderDecorator->setPen(QPen(Qt::red, 1.5));
    followerDecorator->setPen(QPen(Qt::blue, 1.5));
    secondFollowerDecorator->setPen(QPen(QColor(0xCC, 0xAA, 0x00), 1.5));

    // 拖拽由装饰器接管：禁用矩形的鼠标与移动标志，启用装饰器的左键拖拽。
    auto enableDecoratorDrag = [](BroadItem::AnchorDecorator* dec,
                                  QGraphicsObject* rect) {
        rect->setAcceptedMouseButtons(Qt::NoButton);
        rect->setFlag(QGraphicsItem::ItemIsMovable, false);
        dec->setAcceptedMouseButtons(Qt::LeftButton);
        dec->setFlag(QGraphicsItem::ItemIsMovable, true);
    };
    enableDecoratorDrag(leaderDecorator, leader);
    enableDecoratorDrag(followerDecorator, follower);
    enableDecoratorDrag(secondFollowerDecorator, secondFollower);

    // FollowBinding 装饰器→装饰器：拖动 leader 时级联带动整条链。
    BroadItem::FollowBinding* followBinding =
        BroadItem::FollowBinding::create(leaderDecorator, followerDecorator,
                                         offset);
    BroadItem::FollowBinding* secondFollowBinding =
        BroadItem::FollowBinding::create(followerDecorator,
                                         secondFollowerDecorator,
                                         secondOffset);

    // 创建两条连接线，通过锚点连接装饰器
    // line1 连接 leaderDecorator 东锚点 → followerDecorator 西锚点
    // line2 连接 followerDecorator 东锚点 → secondFollowerDecorator 西锚点
    auto* line1 = BroadItem::ConnectionLine::create(
        &scene,
        leaderDecorator->anchor(BroadItem::AnchorDecorator::AnchorSide::East),
        followerDecorator->anchor(
            BroadItem::AnchorDecorator::AnchorSide::West));
    auto* line2 = BroadItem::ConnectionLine::create(
        &scene,
        followerDecorator->anchor(
            BroadItem::AnchorDecorator::AnchorSide::East),
        secondFollowerDecorator->anchor(
            BroadItem::AnchorDecorator::AnchorSide::West));

    // 设置不同画笔颜色以区分两条线
    line1->setPen(QPen(Qt::gray, 2.0));
    line2->setPen(QPen(Qt::darkGray, 2.0));

    InteractiveView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle(QStringLiteral(
        "AnchorDecorator 集成示例 (Esc/Q 退出, D 删黄矩形, B 删蓝矩形)"));
    view.resize(620, 420);
    view.show();
    view.setFocus();

    // 按 D 键删除黄色矩形，演示级联销毁装饰器和锚点的生命周期行为。
    //
    // 删除 secondFollower 后，其 AnchorDecorator 通过 QObject::destroyed 信号
    // 触发 deleteLater()，装饰器及其 8 个锚点自动销毁。
    // line2 因端点（锚点）销毁而自动 setVisible(false)。
    QObject::connect(&view, &InteractiveView::deleteRequested,
                     [&secondFollower, line2]() {
        if (secondFollower) {
            // 删除矩形后其装饰器经 QObject::destroyed 信号 deleteLater 级联销毁
            delete secondFollower;
            secondFollower = nullptr;
            qDebug()
                << "yellowRect destroyed (decorator also destroyed), line2 visible="
                << line2->isVisible();
        }
    });

    // 按 B 键删除蓝色矩形（follower），演示中间端点删除对上下游连接线的影响。
    //
    // 删除 follower 后，其 AnchorDecorator 自动销毁，锚点级联销毁。
    // line1（leaderDecorator→followerDecorator）和 line2（followerDecorator→
    // secondFollowerDecorator）均因锚点 destroyed 而自动 setVisible(false)。
    // secondFollower 仍在场景中但不再跟随
    // （secondFollowBinding 的 leader 被销毁后自动失效）。
    QObject::connect(&view, &InteractiveView::deleteBlueRequested,
                     [&follower, line1, line2]() {
        if (follower) {
            // 删除矩形后其装饰器经 QObject::destroyed 信号 deleteLater 级联销毁
            delete follower;
            follower = nullptr;
            qDebug()
                << "blueRect destroyed (decorator also destroyed), line1 visible="
                << line1->isVisible()
                << ", line2 visible="
                << line2->isVisible();
        }
    });

    const int result = QApplication::exec();

    // 矩形、装饰器、锚点、连接线均为场景项（或场景项的子项），随 scene 析构自动释放。
    // 仅 FollowBinding 是 create() 未传 parent 的裸 QObject，不在任何对象树上，
    // 退出前需手工 destroy() 断开信号连接并 delete。
    if (followBinding != nullptr) {
        followBinding->destroy();
        delete followBinding;
    }
    if (secondFollowBinding != nullptr) {
        secondFollowBinding->destroy();
        delete secondFollowBinding;
    }

    return result;
}

#include "main.moc"
