#pragma once

#include <QGraphicsObject>
#include <QVariantMap>
#include <memory>
#include "LayoutContext.h"
#include "PropertyContext.h"

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

class BroadItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit BroadItem(const QString& xmlFilePath,
                       std::shared_ptr<PropertyContext> ctx = nullptr,
                       QGraphicsItem* parent = nullptr);
    explicit BroadItem(int layoutId,
                       std::shared_ptr<PropertyContext> ctx = nullptr,
                       QGraphicsItem* parent = nullptr);
    ~BroadItem() override;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void setPropertyContext(std::shared_ptr<PropertyContext> ctx);

    void setDynamicProperty(const QString& name, const QVariant& value);
    QVariant dynamicProperty(const QString& name) const;
    bool hasDynamicProperty(const QString& name) const;

    void updateLayout();

    PropertyProxy operator[](const QString& key)
    {
        return m_propertyContext ? (*m_propertyContext)[key] : PropertyProxy(nullptr, QString());
    }

private:
    ElementPtr m_rootElement;
    std::shared_ptr<PropertyContext> m_propertyContext;
    LayoutContext m_context;
    QRectF m_boundingRect;

    void setupPropertyContext();
    void buildFromFile(const QString& path);
    void buildFromRegistry(int layoutId);
    void performLayout();
};

} // namespace BroadItem
