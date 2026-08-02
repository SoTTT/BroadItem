#pragma once

/// @file reactive_helpers.h
/// @brief 测试共享辅助：reactive 模块的可观察图形对象与几何容差判定
///        （原散落于各测试文件的重复定义；boundingRect 尺寸参数化）。
///
/// 本头含 Q_OBJECT 类：AUTOMOC 不会自动 moc 被 include 的头文件，
/// 各消费源文件须在末尾包含 "helpers/moc_reactive_helpers.cpp"。

#include <QGraphicsObject>
#include <QPointF>
#include <QSizeF>

#include <cmath>

namespace BroadItem {
namespace TestHelpers {

/// @brief 最小化 QGraphicsObject 具体实现，用于测试 reactive 模块的绑定/连接/装饰逻辑。
///
/// 实现 QGraphicsObject 要求的 boundingRect() 和 paint() 纯虚函数，
/// 在构造函数中设置 ItemSendsGeometryChanges|ItemSendsScenePositionChanges
/// 标志，使得 Qt 内部 NOTIFY 信号（xChanged/yChanged 等）正常发射。
/// boundingRect 尺寸按测试需要参数化（缺省 100×100）。
class TestObservableObject : public QGraphicsObject {
    Q_OBJECT
public:
    explicit TestObservableObject(QGraphicsItem* parent = nullptr)
        : TestObservableObject(QSizeF(100, 100), parent)
    {
    }

    explicit TestObservableObject(const QSizeF& size, QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent), m_size(size)
    {
        setFlags(flags() | QGraphicsItem::ItemSendsGeometryChanges
                         | QGraphicsItem::ItemSendsScenePositionChanges);
    }

    [[nodiscard]] QRectF boundingRect() const override { return {0, 0, m_size.width(), m_size.height()}; }
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}

private:
    QSizeF m_size;  ///< boundingRect 尺寸（构造时指定）。
};

/// @brief 判断两个 QPointF 在容差范围内是否相等。
/// @param a 第一个点。
/// @param b 第二个点。
/// @param delta 允许的绝对误差（默认 0.5px）。
/// @return true 表示两点在容差内相等。
inline bool pointsNear(const QPointF& a, const QPointF& b, qreal delta = 0.5)
{
    return std::abs(a.x() - b.x()) <= delta && std::abs(a.y() - b.y()) <= delta;
}

} // namespace TestHelpers
} // namespace BroadItem
