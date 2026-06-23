#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QTimer>

#include <broaditem/reactive/ObservableGraphicsObject.h>
#include <broaditem/reactive/ReactiveBinding.h>
#include <broaditem/reactive/ReactiveProperty.h>

/// @brief 示例用的彩色矩形项，继承 ObservableGraphicsObject 以支持属性绑定。
class ColoredRect : public BroadItem::ObservableGraphicsObject {
public:
    /// @brief 构造彩色矩形项。
    /// @param color 填充颜色。
    /// @param width 矩形宽度。
    /// @param height 矩形高度。
    /// @param parent 父项。
    explicit ColoredRect(const QColor& color, qreal width, qreal height,
                         QGraphicsItem* parent = nullptr)
        : BroadItem::ObservableGraphicsObject(parent)
        , m_color(color)
        , m_width(width)
        , m_height(height)
    {
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

/// @brief 入口点。演示两个 Item 通过 ReactiveBinding 实现位置跟随。
///
/// 红色矩形作为领导者，蓝色矩形通过绑定跟随，并带有一个固定偏移量。
/// 使用 QTimer 周期性移动红色矩形，蓝色矩形会自动同步位置。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 600, 400);

    // 创建领导者和跟随者两个彩色矩形
    auto* leader = new ColoredRect(Qt::red, 60, 60);
    auto* follower = new ColoredRect(Qt::blue, 60, 60);

    leader->setPos(50, 170);
    follower->setPos(50, 170);

    scene.addItem(leader);
    scene.addItem(follower);

    // 创建绑定：leader.pos → follower.pos，并添加 (120, 80) 的偏移变换
    auto offsetTransform = [](const QVariant& value) -> QVariant {
        return value.toPointF() + QPointF(120, 80);
    };

    BroadItem::ReactiveBinding* binding =
        BroadItem::ReactiveBinding::create(leader, BroadItem::Property::Pos,
                                           follower, BroadItem::Property::Pos,
                                           offsetTransform);

    // 使用定时器让领导者左右移动，跟随者会自动跟随
    auto* timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, [&]() {
        // 在场景宽度范围内做往复运动
        qreal x = leader->pos().x() + 5.0;
        if (x > 450.0) {
            x = 50.0;
        }
        leader->setPos(x, leader->pos().y());
    });
    timer->start(50);

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle(QStringLiteral("Reactive Binding 跟随示例"));
    view.resize(620, 420);
    view.show();

    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    const int result = app.exec();

    // 清理绑定和定时器
    if (binding != nullptr) {
        binding->destroy();
        delete binding;
    }
    delete timer;

    return result;
}
