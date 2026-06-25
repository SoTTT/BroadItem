#include <QApplication>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>

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
        QGraphicsView::keyPressEvent(event);
    }
};

/// @brief 入口点。演示三个 Item 通过 FollowBinding 实现位置跟随。
///
/// 红色矩形可通过鼠标拖动，蓝色矩形通过 FollowBinding 自动跟随并保持相对偏移；
/// 拖动蓝色矩形可调整两者的相对位置，之后拖动红色矩形时，蓝色矩形会保持新的相对距离同步移动。
/// 黄色矩形跟随蓝色矩形，形成 red → blue → yellow 的跟随链。
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

    BroadItem::FollowBinding* followBinding =
        BroadItem::FollowBinding::create(leader, follower, offset);
    BroadItem::FollowBinding* secondFollowBinding =
        BroadItem::FollowBinding::create(follower, secondFollower, secondOffset);

    // 创建两条连接线，绘制矩形端点之间的连线
    // line1 连接红色(leader) → 蓝色(follower)
    // line2 连接蓝色(follower) → 黄色(secondFollower)
    auto* line1 = BroadItem::ConnectionLine::create(&scene, leader, follower);
    auto* line2 = BroadItem::ConnectionLine::create(&scene, follower, secondFollower);

    // 设置不同画笔颜色以区分两条线
    line1->setPen(QPen(Qt::gray, 2.0));
    line2->setPen(QPen(Qt::darkGray, 2.0));

    InteractiveView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle(QStringLiteral("Follow Binding 跟随示例 (Esc/Q 退出, D 删黄矩形)"));
    view.resize(620, 420);
    view.show();
    view.setFocus();

    // 按 D 键删除黄色矩形，演示端点删除后连接线自动隐藏的生命周期行为
    QObject::connect(&view, &InteractiveView::deleteRequested, [&]() {
        if (secondFollower) {
            delete secondFollower;
            secondFollower = nullptr;
            qDebug() << "yellowRect destroyed, line2 visible="
                     << (line2 ? line2->isVisible() : false);
        }
    });

    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    const int result = app.exec();

    // 清理：先删连接线，再删 FollowBinding，最后处理矩形（secondFollower 可能已通过 D 键删除）
    if (line1) {
        delete line1;
        line1 = nullptr;
    }
    if (line2) {
        delete line2;
        line2 = nullptr;
    }

    // 清理绑定
    if (followBinding != nullptr) {
        followBinding->destroy();
        delete followBinding;
        followBinding = nullptr;
    }
    if (secondFollowBinding != nullptr) {
        secondFollowBinding->destroy();
        delete secondFollowBinding;
        secondFollowBinding = nullptr;
    }

    // 清理矩形（secondFollower 可能已通过 D 键删除，判空跳过）
    if (leader) {
        delete leader;
        leader = nullptr;
    }
    if (follower) {
        delete follower;
        follower = nullptr;
    }
    if (secondFollower) {
        delete secondFollower;
        secondFollower = nullptr;
    }

    return result;
}

#include "main.moc"
