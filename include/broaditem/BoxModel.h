#pragma once

#include <QColor>
#include <QFont>
#include <QMarginsF>
#include <QRectF>
#include <QString>

namespace BroadItem {

struct Padding {
    double top = 0;
    double bottom = 0;
    double left = 0;
    double right = 0;

    double width() const { return left + right; }
    double height() const { return top + bottom; }
};

struct Margin {
    double top = 0;
    double bottom = 0;
    double left = 0;
    double right = 0;

    double width() const { return left + right; }
    double height() const { return top + bottom; }
};

struct Border {
    double radius = 0;
    double width = 0;
    QColor color = Qt::black;
    QString style = "none";

    bool visible() const { return style != "none" && width > 0; }
};

struct Background {
    QColor color = Qt::white;
    double radius = 0;
    double transparent = 0;
    bool enabled = false;

    bool visible() const { return enabled && transparent < 1.0; }
};

struct Decorators {
    Margin margin;
    Border border;
    Background background;
    Padding padding;

    double totalWidth() const {
        return margin.width() + border.width * 2 + padding.width();
    }
    double totalHeight() const {
        return margin.height() + border.width * 2 + padding.height();
    }
};

} // namespace BroadItem
