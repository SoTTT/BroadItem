#pragma once

#include <QColor>
#include <QFont>
#include <QMarginsF>
#include <QRectF>
#include <QString>

namespace BroadItem {

/// @brief 表示内容周围的填充尺寸。
struct Padding {
    double top = 0;
    double bottom = 0;
    double left = 0;
    double right = 0;

    /// @brief 返回水平总填充（左+右）。
    double width() const { return left + right; }
    /// @brief 返回垂直总填充（上+下）。
    double height() const { return top + bottom; }
};

/// @brief 表示元素周围的边距尺寸。
struct Margin {
    double top = 0;
    double bottom = 0;
    double left = 0;
    double right = 0;

    /// @brief 返回水平总边距（左+右）。
    double width() const { return left + right; }
    /// @brief 返回垂直总边距（上+下）。
    double height() const { return top + bottom; }
};

/// @brief 表示元素边框（圆角、宽度、颜色和样式）。
struct Border {
    double radius = 0;
    double width = 0;
    QColor color = Qt::black;
    QString style = "none";

    /// @brief 如果边框可见则返回 true。
    bool visible() const { return style != "none" && width > 0; }
};

/// @brief 表示背景填充（颜色、圆角和透明度）。
struct Background {
    QColor color = Qt::white;
    double radius = 0;
    double opacity = 1;
    bool enabled = false;

    /// @brief 如果背景可见则返回 true。
    bool visible() const { return enabled && opacity > 0; }
};

} // namespace BroadItem
