#pragma once

#include <QGraphicsItem>
#include <QVariantMap>
#include <memory>
#include "LayoutContext.h"

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

class BroadItem : public QGraphicsItem {
public:
    explicit BroadItem(const QString& xmlFilePath, QGraphicsItem* parent = nullptr);
    explicit BroadItem(int layoutId, QGraphicsItem* parent = nullptr);
    ~BroadItem();

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void setDynamicProperty(const QString& name, const QVariant& value);
    QVariant dynamicProperty(const QString& name) const;
    bool hasDynamicProperty(const QString& name) const;

    void updateLayout();

private:
    ElementPtr m_rootElement;
    LayoutContext m_context;
    QRectF m_boundingRect;

    void buildFromFile(const QString& path);
    void buildFromRegistry(int layoutId);
    void performLayout();
};

} // namespace BroadItem
