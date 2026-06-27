#pragma once

#include <QGraphicsObject>
#include <QPen>
#include <QMetaObject>

class QGraphicsScene;

namespace BroadItem {

class AnchorPoint;
class ReactiveBinding;

/// @brief AnchorDecorator — 装饰器图元，为任意 QGraphicsObject 包围 8 方向锚点和外框。
///
/// AnchorDecorator 包裹一个被装饰的 QGraphicsObject（decorated），成为其
/// QGraphicsItem 父节点，使得 decorated 随装饰器整体平移。锚点（AnchorPoint）
/// 保持为顶级场景图元（QGraphicsItem parent = nullptr），其 scenePos 可直接
/// 被 ConnectionLine 读取。
///
/// 装饰器通过 ReactiveBinding::createObserver 监听 decorated 的 pos 属性变化
/// 以实现几何体实时跟动；在 decorated 具备带 NOTIFY 信号的 width/height 属性时
/// 额外连接这两个属性的观察者。
///
/// 生命周期：
/// - 通过静态工厂 create() 构造，参数无效时返回 nullptr 并输出 qWarning。
/// - 析构时将 decorated 重新挂回场景顶级（保持场景坐标），删除 8 个锚点，
///   销毁所有 observer 绑定。
/// - 当 decorated 被销毁时，装饰器通过 deleteLater() 自我调度删除。
///
/// 限制：
/// - 不参与 XML 元素树和 measure/layout/render 管线。
/// - 不处理鼠标/键盘交互。
/// - MVP 假定 decorated 为顶级场景图元（pos == scenePos），依赖该假设
///   计算初始 sceneBoundingRect 和 anchor scenePos。
class AnchorDecorator : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(qreal width READ width NOTIFY widthChanged)
    Q_PROPERTY(qreal height READ height NOTIFY heightChanged)

public:
    /// @brief 锚点方位枚举，对应 8 个罗盘方向。
    enum class AnchorSide {
        North,      ///< 正北（上）。
        NorthEast,  ///< 东北（右上）。
        East,       ///< 正东（右）。
        SouthEast,  ///< 东南（右下）。
        South,      ///< 正南（下）。
        SouthWest,  ///< 西南（左下）。
        West,       ///< 正西（左）。
        NorthWest   ///< 西北（左上）。
    };

    /// @brief 静态工厂方法，创建 AnchorDecorator 实例。
    ///
    /// 校验流程：
    /// 1. scene 非空
    /// 2. decorated 非空
    /// 3. decorated->parentItem() 为空（尚未被其他图元父化）
    /// 校验失败时输出 qWarning 并返回 nullptr。
    ///
    /// 构造流程：
    /// 1. new AnchorDecorator(scene, decorated, parent)
    /// 2. scene->addItem(this)
    /// 3. decorated->setParentItem(this)
    /// 4. setZValue(1)
    /// 5. 创建 ReactiveBinding observer 监听 pos（始终）
    /// 6. 预检 decorated 的 width/height 属性，仅在其具备 NOTIFY 信号时创建 observer
    /// 7. 连接 decorated::destroyed 信号以触发 deleteLater
    /// 8. 调用 computeAndApplyGeometry() 初始布局
    ///
    /// @param scene 目标 QGraphicsScene，必须非空。
    /// @param decorated 被装饰的 QGraphicsObject，必须非空且无 parentItem()。
    /// @param parent 可选的 QObject 父对象。
    /// @return AnchorDecorator* 新实例；参数无效时返回 nullptr。
    static AnchorDecorator* create(QGraphicsScene* scene,
                                   QGraphicsObject* decorated,
                                   QObject* parent = nullptr);

    /// @brief 析构函数。
    ///
    /// 执行顺序：
    /// 1. 断开 m_decoratedDestroyConnection
    /// 2. 若 decorated 仍存活：reparent 回场景顶级并保持 scenePos
    /// 3. 删除 8 个 AnchorPoint 实例
    /// 4. destroy + delete 所有 ReactiveBinding observer
    ~AnchorDecorator() override;

    /// @brief 获取指定方位的锚点。
    /// @param side 锚点方位。
    /// @return AnchorPoint* 对应锚点指针。
    [[nodiscard]] AnchorPoint* anchor(AnchorSide side) const;

    /// @brief 设置装饰器外框画笔。
    /// @param pen 新的 QPen。
    void setPen(const QPen& pen);

    /// @brief 获取外框画笔。
    /// @return 当前 QPen。
    [[nodiscard]] QPen pen() const;

    /// @brief 设置边距——装饰器外框与内容之间的空白区域。
    /// @param margin 新边距值（像素），触发 computeAndApplyGeometry()。
    void setMargin(qreal margin);

    /// @brief 获取当前边距。
    /// @return 边距值（像素）。
    [[nodiscard]] qreal margin() const;

    /// @brief 设置所有 8 个锚点的可见性。
    /// @param visible true 显示，false 隐藏。
    void setAnchorVisible(bool visible);

    /// @brief 获取锚点可见性（以第一个锚点为准）。
    /// @return true 表示锚点可见。
    [[nodiscard]] bool anchorVisible() const;

    /// @brief 获取装饰器当前宽度。
    /// @return 宽度值（像素）。
    [[nodiscard]] qreal width() const;

    /// @brief 获取装饰器当前高度。
    /// @return 高度值（像素）。
    [[nodiscard]] qreal height() const;

    /// @brief 返回装饰器的边界矩形。
    /// @return QRectF(0, 0, m_width, m_height)。
    [[nodiscard]] QRectF boundingRect() const override;

    /// @brief 绘制装饰器外框——m_pen 描边矩形，NoBrush 填充。
    /// @param painter QPainter 实例。
    /// @param option 样式选项（忽略）。
    /// @param widget 目标 widget（忽略）。
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

public slots:
    /// @brief 强制重新同步几何体。
    ///
    /// 调用 computeAndApplyGeometry() 重新计算装饰器位置、大小和锚点位置。
    /// 适用于 decorated 发生变换（如缩放/旋转）后，需要重新计算
    /// sceneBoundingRect() 以更新轴对齐包围盒的场景。
    void resync();

signals:
    /// @brief 宽度变化信号。
    void widthChanged();
    /// @brief 高度变化信号。
    void heightChanged();

private:
    /// @brief 私有构造，通过 create() 工厂方法创建。
    /// @param scene 目标场景。
    /// @param decorated 被装饰对象。
    /// @param parent QObject 父对象。
    AnchorDecorator(QGraphicsScene* scene,
                    QGraphicsObject* decorated,
                    QObject* parent);

    /// @brief 计算并应用装饰器几何体。
    ///
    /// 执行步骤：
    /// 1. 守卫：m_decorated 为空则直接返回
    /// 2. 获取 m_decorated->sceneBoundingRect() 作为轴对齐包围盒
    /// 3. 设置装饰器自身 scenePos 为 sceneAabb.topLeft() - (m_margin, m_margin)
    /// 4. 调整 decorated 在装饰器内的局部坐标
    /// 5. 更新 m_width/m_height，仅在变化时发射 widthChanged/heightChanged
    /// 6. 调用 updateAnchorPositions()
    /// 7. prepareGeometryChange() + update()
    void computeAndApplyGeometry();

    /// @brief 根据当前 m_width/m_height 重新计算 8 个锚点的位置。
    void updateAnchorPositions();

    /// @brief 检查 QObject 的指定属性是否有 NOTIFY 信号。
    ///
    /// 用于预检 decorated 的 width/height 属性是否支持响应式监听，
    /// 避免调用 ReactiveBinding::createObserver 时输出 qWarning。
    /// @param obj 要检查的 QObject。
    /// @param propName 属性名。
    /// @return true 表示属性存在且有 NOTIFY 信号。
    static bool hasNotifyProperty(const QObject* obj, const QString& propName);

    QGraphicsObject* m_decorated;               ///< 被装饰的 QGraphicsObject。
    AnchorPoint* m_anchors[8];                   ///< 8 个方位锚点（N/NE/E/SE/S/SW/W/NW）。
    QPen m_pen;                                  ///< 外框画笔。
    qreal m_margin;                              ///< 边距。
    qreal m_width;                               ///< 装饰器宽度。
    qreal m_height;                              ///< 装饰器高度。
    bool m_anchorVisible;                        ///< 锚点可见性标记。
    bool m_inGeometryUpdate;                     ///< 防止 computeAndApplyGeometry 重入标志。

    ReactiveBinding* m_posObserver;              ///< 监听 decorated pos 变化的 observer。
    ReactiveBinding* m_widthObserver;            ///< 监听 decorated width 变化的 observer（可选）。
    ReactiveBinding* m_heightObserver;           ///< 监听 decorated height 变化的 observer（可选）。

    QMetaObject::Connection m_decoratedDestroyConnection; ///< decorated 销毁信号连接。
};

} // namespace BroadItem
