#pragma once

#include <QVariantMap>
#include <QVariant>
#include <QStringList>
#include "BoxModel.h"

namespace BroadItem {

class LayoutContext {
public:
    QVariantMap dynamicProperties;

    bool hasProperty(const QString& name) const {
        return dynamicProperties.contains(name);
    }

    QString getString(const QString& name) const {
        return dynamicProperties.value(name).toString();
    }

    QStringList getStringList(const QString& name) const {
        return dynamicProperties.value(name).toStringList();
    }

    void setProperty(const QString& name, const QVariant& value) {
        dynamicProperties.insert(name, value);
    }
};

struct MeasureResult {
    Size intrinsicSize;
};

struct LayoutConstraints {
    double availableWidth = -1;  // -1 means unconstrained
    double availableHeight = -1;
};

} // namespace BroadItem
