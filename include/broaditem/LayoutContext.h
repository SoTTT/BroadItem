#pragma once

#include <QVariant>
#include <QStringList>
#include "BoxModel.h"
#include "PropertyContext.h"

namespace BroadItem {

class LayoutContext {
public:
    PropertyContext* ctx = nullptr;

    bool hasProperty(const QString& name) const
    {
        return ctx && ctx->hasProperty(name);
    }

    QVariant property(const QString& name) const
    {
        return ctx ? ctx->property(name) : QVariant();
    }

    void setProperty(const QString& name, const QVariant& value)
    {
        if (ctx)
            ctx->setProperty(name, value);
    }
};

struct MeasureResult {
    Size intrinsicSize;
};

struct LayoutConstraints {
    double availableWidth = -1;
    double availableHeight = -1;
};

} // namespace BroadItem
