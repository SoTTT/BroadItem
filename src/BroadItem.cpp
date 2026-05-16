#include "broaditem/BroadItem.h"
#include "broaditem/Element.h"
#include "broaditem/MapPropertyContext.h"
#include "broaditem/XmlLayoutParser.h"
#include "broaditem/LayoutRegistry.h"
#include "broaditem/LayoutEngine.h"
#include <QPainter>

namespace BroadItem {

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

BroadItem::~BroadItem() = default;

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

void BroadItem::setPropertyContext(std::shared_ptr<PropertyContext> ctx)
{
    m_propertyContext = std::move(ctx);
    setupPropertyContext();
}

void BroadItem::buildFromFile(const QString& path)
{
    m_rootElement = XmlLayoutParser::parseFile(path);
}

void BroadItem::buildFromRegistry(int layoutId)
{
    auto root = LayoutRegistry::instance().getLayout(layoutId);
    if (root) {
        m_rootElement = root;
    }
}

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

QRectF BroadItem::boundingRect() const
{
    return m_boundingRect;
}

void BroadItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    if (m_rootElement) {
        m_context.ctx = m_propertyContext.get();
        m_rootElement->render(painter, m_context);
    }
}

void BroadItem::setDynamicProperty(const QString& name, const QVariant& value)
{
    if (m_propertyContext)
        m_propertyContext->setProperty(name, value);
}

QVariant BroadItem::dynamicProperty(const QString& name) const
{
    return m_propertyContext ? m_propertyContext->property(name) : QVariant();
}

bool BroadItem::hasDynamicProperty(const QString& name) const
{
    return m_propertyContext && m_propertyContext->hasProperty(name);
}

void BroadItem::updateLayout()
{
    prepareGeometryChange();
    performLayout();
}

} // namespace BroadItem
