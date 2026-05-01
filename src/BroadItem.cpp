#include "broaditem/BroadItem.h"
#include "broaditem/Element.h"
#include "broaditem/XmlLayoutParser.h"
#include "broaditem/LayoutRegistry.h"
#include "broaditem/LayoutEngine.h"
#include <QPainter>
#include <QDebug>

namespace BroadItem {

BroadItem::BroadItem(const QString& xmlFilePath, QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
    buildFromFile(xmlFilePath);
    performLayout();
}

BroadItem::BroadItem(int layoutId, QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
    buildFromRegistry(layoutId);
    performLayout();
}

BroadItem::~BroadItem() = default;

void BroadItem::buildFromFile(const QString& path)
{
    m_rootElement = XmlLayoutParser::parseFile(path);
}

void BroadItem::buildFromRegistry(int layoutId)
{
    auto root = LayoutRegistry::instance().getLayout(layoutId);
    if (root) {
        // Clone the template root for this instance
        // For simplicity, we reuse the same element (not safe for multiple BroadItems)
        // A proper clone() method should be implemented for production use.
        m_rootElement = root;
    }
}

void BroadItem::performLayout()
{
    if (!m_rootElement)
        return;

    LayoutConstraints constraints;
    constraints.availableWidth = -1;
    constraints.availableHeight = -1;

    auto result = m_rootElement->measure(m_context, constraints);
    m_boundingRect = QRectF(0, 0, result.intrinsicSize.width, result.intrinsicSize.height);

    Rect rootRect;
    rootRect.pos.x = 0;
    rootRect.pos.y = 0;
    rootRect.size.width = result.intrinsicSize.width;
    rootRect.size.height = result.intrinsicSize.height;
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
    if (m_rootElement)
        m_rootElement->render(painter, m_context);
}

void BroadItem::setDynamicProperty(const QString& name, const QVariant& value)
{
    // Validate type: only QString and QStringList are supported
    if (!value.canConvert<QString>() && value.type() != QVariant::StringList) {
        qWarning() << "BroadItem: dynamic property '" << name
                   << "' must be QString or QStringList, got type" << value.typeName();
        return;
    }

    bool changed = false;
    QVariant oldValue = m_context.dynamicProperties.value(name);
    if (oldValue != value) {
        changed = true;
        m_context.dynamicProperties.insert(name, value);
    }

    if (changed && m_rootElement && m_rootElement->bindsProperty(name)) {
        updateLayout();
        update();
    }
}

QVariant BroadItem::dynamicProperty(const QString& name) const
{
    return m_context.dynamicProperties.value(name);
}

bool BroadItem::hasDynamicProperty(const QString& name) const
{
    return m_context.dynamicProperties.contains(name);
}

void BroadItem::updateLayout()
{
    prepareGeometryChange();
    performLayout();
}

} // namespace BroadItem
