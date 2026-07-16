#include <broaditem/core/BroadItem.h>
#include <QPainter>

namespace BroadItem {

/// @brief 从 XML 文件路径构造 BroadItem。
/// @param xmlFilePath Path to the XML layout file.
/// @param ctx Optional property context; Frame creates default if null.
/// @param parent Optional QGraphicsItem parent.
BroadItem::BroadItem(const QString& xmlFilePath,
                     std::shared_ptr<PropertyContext> ctx,
                     QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_frame(Frame::fromFile(xmlFilePath, std::move(ctx)))
{
    setupPropertyContext();
    setFlags(flags() | QGraphicsItem::ItemSendsGeometryChanges
                     | QGraphicsItem::ItemSendsScenePositionChanges);
}

/// @brief 从注册的布局 ID 构造 BroadItem。
/// @param layoutId Identifier of a pre-registered layout.
/// @param ctx Optional property context; Frame creates default if null.
/// @param parent Optional QGraphicsItem parent.
BroadItem::BroadItem(int layoutId,
                     std::shared_ptr<PropertyContext> ctx,
                     QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_frame(Frame::fromRegistry(layoutId, std::move(ctx)))
{
    setupPropertyContext();
    setFlags(flags() | QGraphicsItem::ItemSendsGeometryChanges
                     | QGraphicsItem::ItemSendsScenePositionChanges);
}

/// @brief 析构函数。
BroadItem::~BroadItem() = default;

/// @brief 连接属性上下文变更回调，在绑定的属性变化时触发 Frame 重新布局并通知 QGraphicsItem 重绘。
///
/// 覆盖 Frame 内部注册的回调，额外调用 prepareGeometryChange() 和 update()
/// 以确保 QGraphicsScene 正确反映几何变更。
void BroadItem::setupPropertyContext()
{
    auto ctx = m_frame->propertyContext();
    if (ctx) {
        ctx->setOnChanged([this](const QString& name, const QVariant&) {
            if (m_frame->bindsProperty(name)) {
                m_frame->invalidate();
                prepareGeometryChange();
                m_frame->performLayout();
                update();
            }
        });
    }
}

/// @brief 替换属性上下文并重新连接变更通知。
/// @param ctx The new property context.
void BroadItem::setPropertyContext(std::shared_ptr<PropertyContext> ctx)
{
    m_frame->setPropertyContext(std::move(ctx));
    setupPropertyContext();
}

/// @brief 设置动态属性值，如已绑定则通过 onChanged 回调触发重新布局。
/// @param name Property name.
/// @param value Property value.
void BroadItem::setDynamicProperty(const QString& name, const QVariant& value)
{
    m_frame->setDynamicProperty(name, value);
}

/// @brief 返回动态属性值。
/// @param name Property name.
/// @return The property value, or invalid QVariant if not found.
QVariant BroadItem::dynamicProperty(const QString& name) const
{
    return m_frame->dynamicProperty(name);
}

/// @brief 检查动态属性是否存在。
/// @param name Property name.
/// @return True if the property context has the property.
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
/// @return The computed bounding rect from the layout phase.
QRectF BroadItem::boundingRect() const
{
    return QRectF(QPointF(), m_frame->size());
}

/// @brief 将元素树绘制到指定的 painter 上。
/// @param painter The QPainter to render onto.
/// @param option Style options (unused).
/// @param widget Widget (unused).
void BroadItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    m_frame->paint(painter);
}

} // namespace BroadItem
