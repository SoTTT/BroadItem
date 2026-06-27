/// @file AnchorPoint.cpp
/// @brief AnchorPoint 实现 —— 顶级场景锚点项，渲染为实心圆。

#include <broaditem/reactive/AnchorPoint.h>

#include <QGraphicsScene>
#include <QPainter>

namespace BroadItem {

// ══════════════════════════════════════════════════════════════════
// 构造
// ══════════════════════════════════════════════════════════════════

AnchorPoint::AnchorPoint(QGraphicsScene* scene, QObject* parent)
    : QGraphicsObject(nullptr)          // QGraphicsItem parent 为 nullptr（顶级场景 item）
    , m_diameter(kDefaultDiameter)
    , m_color(0x55, 0x55, 0x55)        // #555555
{
    // 设置 QObject parent（通常为 AnchorDecorator）
    if (parent) {
        QObject::setParent(parent);
    }

    // 将自身注册到场景中
    scene->addItem(this);

    // 非交互式：禁用所有鼠标交互、选中和拖拽
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, false);

    // 锚点绘制在连接线（zValue=1）之上
    setZValue(2);
}

// ══════════════════════════════════════════════════════════════════
// Getter / Setter
// ══════════════════════════════════════════════════════════════════

qreal AnchorPoint::diameter() const
{
    return m_diameter;
}

void AnchorPoint::setDiameter(qreal d)
{
    // 杜绝零值：取绝对值并使用 1 作为最小兜底
    if (d < 0) d = -d;
    if (d < 1.0) d = 1.0;

    if (qFuzzyCompare(m_diameter, d)) return;

    // 通知 scene 几何即将改变，使 QGraphicsView 能正确刷新旧区域
    prepareGeometryChange();
    m_diameter = d;
    update();
}

QColor AnchorPoint::color() const
{
    return m_color;
}

void AnchorPoint::setColor(const QColor& color)
{
    if (m_color == color) return;
    m_color = color;
    update();
}

bool AnchorPoint::anchorVisible() const
{
    return isVisible();
}

void AnchorPoint::setAnchorVisible(bool visible)
{
    setVisible(visible);
}

// ══════════════════════════════════════════════════════════════════
// QGraphicsItem 虚函数
// ══════════════════════════════════════════════════════════════════

QRectF AnchorPoint::boundingRect() const
{
    qreal half = m_diameter / 2.0;
    return {-half, -half, m_diameter, m_diameter};
}

void AnchorPoint::paint(QPainter* painter,
                        const QStyleOptionGraphicsItem*,
                        QWidget*)
{
    // 设置抗锯齿以绘制圆滑的圆形
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF ellipseRect = boundingRect();

    // 实心填充
    painter->setBrush(m_color);

    // 1px 黑色描边，增强可辨识度
    painter->setPen(QPen(Qt::black, 1.0));

    painter->drawEllipse(ellipseRect);
}

} // namespace BroadItem
