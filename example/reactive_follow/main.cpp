#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>

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
        setFlags(flags() | QGraphicsItem::ItemIsMovable);
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
/// 红色矩形可通过鼠标拖动，蓝色矩形通过绑定自动跟随，并带有一个固定偏移量。拖动红色矩形时，蓝色矩形会实时同步位置。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 600, 400);

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

    QGraphicsView view(&scene);
    view.setRenderHints(QPainter::Antialiasing);
    view.setWindowTitle(QStringLiteral("Reactive Binding 跟随示例"));
    view.resize(620, 420);
    view.show();

    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    const int result = app.exec();

    // 清理绑定
    if (binding != nullptr) {
        binding->destroy();
        delete binding;
    }

    return result;
}
