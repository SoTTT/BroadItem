#include <broaditem/core/BroadItem.h>
#include <QPainter>

namespace BroadItem {

/// @brief 从 XML 文件路径构造 BroadItem。
/// @param xmlFilePath XML 布局文件路径。
/// @param ctx 属性上下文，为 nullptr 时由 Frame 创建默认上下文。
/// @param parent 可选的 QGraphicsItem 父项。
BroadItem::BroadItem(const QString& xmlFilePath,
                     std::shared_ptr<PropertyContext> ctx,
                     QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_frame(Frame::fromFile(xmlFilePath, std::move(ctx)))
{
    init();
}

/// @brief 从注册的布局 ID 构造 BroadItem。
/// @param layoutId 预注册布局的标识符。
/// @param ctx 属性上下文，为 nullptr 时由 Frame 创建默认上下文。
/// @param parent 可选的 QGraphicsItem 父项。
BroadItem::BroadItem(int layoutId,
                     std::shared_ptr<PropertyContext> ctx,
                     QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_frame(Frame::fromRegistry(layoutId, std::move(ctx)))
{
    init();
}

/// @brief 析构函数。
BroadItem::~BroadItem() = default;

/// @brief 构造函数共用初始化：注入重布局动作并开启几何/场景位置变更通知，
///        使 Qt 内部 Q_PROPERTY 的 NOTIFY 信号可发射。
void BroadItem::init()
{
    setupRelayoutAction();
    setFlags(flags() | QGraphicsItem::ItemSendsGeometryChanges
                     | QGraphicsItem::ItemSendsScenePositionChanges);
}

/// @brief 向 Frame 注入重布局动作：布局执行前通知 QGraphicsScene 几何变更，布局后触发重绘。
///
/// 不再覆盖上下文的 onChanged 回调（由 Frame 统一持有并按更新策略调度），
/// 仅替换 Frame 的重布局动作；动作捕获的 this 为 BroadItem 成员 m_frame 的
/// 持有方，Frame 销毁时其待定合并任务随之取消，不会回调到已销毁的 BroadItem。
void BroadItem::setupRelayoutAction()
{
    m_frame->setRelayoutAction([this] {
        prepareGeometryChange();
        m_frame->performLayout();
        update();
    });
}

/// @brief 替换属性上下文并重新连接变更通知。
/// @param ctx 新的属性上下文。
void BroadItem::setPropertyContext(std::shared_ptr<PropertyContext> ctx)
{
    m_frame->setPropertyContext(std::move(ctx));
    setupRelayoutAction();
}

/// @brief 设置动态属性值，如已绑定则通过 onChanged 回调触发重新布局。
/// @param name 属性名。
/// @param value 属性值。
void BroadItem::setDynamicProperty(const QString& name, const QVariant& value)
{
    m_frame->setDynamicProperty(name, value);
}

/// @brief 返回动态属性值。
/// @param name 属性名。
/// @return 属性值；未找到时返回无效 QVariant。
QVariant BroadItem::dynamicProperty(const QString& name) const
{
    return m_frame->dynamicProperty(name);
}

/// @brief 检查动态属性是否存在。
/// @param name 属性名。
/// @return 属性上下文中存在该属性时返回 true。
bool BroadItem::hasDynamicProperty(const QString& name) const
{
    return m_frame->hasDynamicProperty(name);
}

/// @brief 通过通知几何变化并重新运行管线来触发完整布局更新。
void BroadItem::updateLayout()
{
    prepareGeometryChange();
    m_frame->performLayout();
}

/// @brief 返回项目的边界矩形。
/// @return 布局阶段计算出的边界矩形。
QRectF BroadItem::boundingRect() const
{
    return QRectF(QPointF(), m_frame->size());
}

/// @brief 将元素树绘制到指定的 painter 上。
/// @param painter 目标 QPainter。
/// @param option 样式选项（未使用）。
/// @param widget 目标 Widget（未使用）。
void BroadItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    m_frame->paint(painter);
}

} // namespace BroadItem
