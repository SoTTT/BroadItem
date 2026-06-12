#include <broaditem/core/BroadItem.h>
#include <broaditem/element/Element.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/parser/LayoutRegistry.h>
#include <broaditem/core/LayoutEngine.h>
#include <QPainter>

namespace BroadItem {

/// @brief 从 XML 文件路径构造 BroadItem。
/// @param xmlFilePath Path to the XML layout file.
/// @param ctx Optional property context; uses MapPropertyContext if null.
/// @param parent Optional QGraphicsItem parent.
BroadItem::BroadItem(const QString& xmlFilePath,
                     std::shared_ptr<PropertyContext> ctx,
                     QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_propertyContext(ctx ? std::move(ctx) : std::make_shared<MapPropertyContext>())
{
    setupPropertyContext();
    buildFromFile(xmlFilePath);
    performLayout();
}

/// @brief 从注册的布局 ID 构造 BroadItem。
/// @param layoutId Identifier of a pre-registered layout.
/// @param ctx Optional property context; uses MapPropertyContext if null.
/// @param parent Optional QGraphicsItem parent.
BroadItem::BroadItem(int layoutId,
                     std::shared_ptr<PropertyContext> ctx,
                     QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_propertyContext(ctx ? std::move(ctx) : std::make_shared<MapPropertyContext>())
{
    setupPropertyContext();
    buildFromRegistry(layoutId);
    performLayout();
}

/// @brief 析构函数。
BroadItem::~BroadItem() = default;

/// @brief 连接属性上下文变更回调，在绑定的属性变化时触发重新布局。
void BroadItem::setupPropertyContext()
{
    if (m_propertyContext) {
        m_propertyContext->setOnChanged([this](const QString& name, const QVariant&) {
            if (m_rootElement && m_rootElement->bindsProperty(name)) {
                updateLayout();
                update();
            }
        });
    }
}

/// @brief 替换属性上下文并重新连接变更通知。
/// @param ctx The new property context.
void BroadItem::setPropertyContext(std::shared_ptr<PropertyContext> ctx)
{
    m_propertyContext = std::move(ctx);
    setupPropertyContext();
}

/// @brief 解析 XML 布局文件并构建元素树。
/// @param path File path to the XML layout.
void BroadItem::buildFromFile(const QString& path)
{
    m_rootElement = XmlLayoutParser::parseFile(path);
}

/// @brief 从 LayoutRegistry 按 ID 加载预注册的布局。
/// @param layoutId Registered layout identifier.
void BroadItem::buildFromRegistry(int layoutId)
{
    auto root = LayoutRegistry::instance().getLayout(layoutId);
    if (root) {
        m_rootElement = root;
    }
}

/// @brief 运行三阶段管线：测量、布局并设置边界矩形。
void BroadItem::performLayout()
{
    if (!m_rootElement)
        return;

    m_context.ctx = m_propertyContext.get();

    LayoutConstraints constraints;
    constraints.availableWidth = -1;
    constraints.availableHeight = -1;

    auto result = m_rootElement->measure(m_context, constraints);
    m_boundingRect = QRectF(0, 0, result.intrinsicSize.width(), result.intrinsicSize.height());

    QRectF rootRect(0, 0, result.intrinsicSize.width(), result.intrinsicSize.height());
    m_rootElement->layout(m_context, rootRect);
}

/// @brief 返回项目的边界矩形。
/// @return The computed bounding rect from the layout phase.
QRectF BroadItem::boundingRect() const
{
    return m_boundingRect;
}

/// @brief 将元素树绘制到指定的 painter 上。
/// @param painter The QPainter to render onto.
/// @param option Style options (unused).
/// @param widget Widget (unused).
void BroadItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    if (m_rootElement) {
        m_context.ctx = m_propertyContext.get();
        m_rootElement->render(painter, m_context);
    }
}

/// @brief 设置动态属性值，如已绑定则触发重新布局。
/// @param name Property name.
/// @param value Property value.
void BroadItem::setDynamicProperty(const QString& name, const QVariant& value)
{
    if (m_propertyContext)
        m_propertyContext->setProperty(name, value);
}

/// @brief 返回动态属性值。
/// @param name Property name.
/// @return The property value, or invalid QVariant if not found.
QVariant BroadItem::dynamicProperty(const QString& name) const
{
    return m_propertyContext ? m_propertyContext->property(name) : QVariant();
}

/// @brief 检查动态属性是否存在。
/// @param name Property name.
/// @return True if the property context has the property.
bool BroadItem::hasDynamicProperty(const QString& name) const
{
    return m_propertyContext && m_propertyContext->hasProperty(name);
}

/// @brief 通过通知几何变化并重新运行管线来触发完整布局更新。
void BroadItem::updateLayout()
{
    prepareGeometryChange();
    performLayout();
}

} // namespace BroadItem
