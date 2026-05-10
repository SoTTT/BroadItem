#pragma once

#include <QPainter>
#include <QDomElement>
#include <QStringList>
#include <memory>
#include <vector>
#include "BoxModel.h"
#include "LayoutContext.h"

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

class Element {
public:
    virtual ~Element() = default;

    // Parse attributes from XML
    virtual void parse(const QDomElement& xml);

    // Measure phase: compute intrinsic size
    virtual MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) = 0;

    // Layout phase: assign final position and size
    virtual void layout(const LayoutContext& ctx, const Rect& rect) = 0;

    // Render
    virtual void render(QPainter* painter, const LayoutContext& ctx) const = 0;

    // Data binding: return true if this element uses the given property
    virtual bool bindsProperty(const QString& name) const { Q_UNUSED(name) return false; }

    // Check if name matches bindPath (exact, or as prefix of a dotted/bracket path)
    static bool matchesProperty(const QString& bindPath, const QString& propName);

    // Clone this element (deep copy). Must be implemented by all concrete element types.
    virtual ElementPtr clone() const = 0;

    // Interpolate placeholder values (e.g. `{}`) into this element and its children.
    // Default implementation does nothing.
    virtual void interpolateValues(const QStringList& values);

    const Rect& rect() const { return m_rect; }
    void setRect(const Rect& r) { m_rect = r; }

    // Decorators (may be null for control elements)
    Decorators decorators;
    bool isControlElement = false;

protected:
    Rect m_rect;

    // Helpers
    double parseDouble(const QString& value, double defaultVal = 0) const;
    QColor parseColor(const QString& value) const;
    bool parseBool(const QString& value) const;
    QString interpolate(const QString& text, const QStringList& values) const;
};

} // namespace BroadItem
