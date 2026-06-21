#pragma once

#include <QString>

namespace BroadItem {

/// @brief 响应式属性键常量，用于将属性名映射到 QGraphicsObject 的 getter/setter。
///
/// ReactiveBinding 使用这些常量将 XML 中的属性绑定到对应图形对象的属性上。
struct Property
{
    /// @brief 位置属性键。
    static const QString Pos;
    /// @brief 缩放属性键。
    static const QString Scale;
    /// @brief 旋转属性键。
    static const QString Rotation;
    /// @brief 透明度属性键。
    static const QString Opacity;
    /// @brief 可见性属性键。
    static const QString Visible;
};

inline const QString Property::Pos      = QStringLiteral("pos");
inline const QString Property::Scale    = QStringLiteral("scale");
inline const QString Property::Rotation = QStringLiteral("rotation");
inline const QString Property::Opacity  = QStringLiteral("opacity");
inline const QString Property::Visible  = QStringLiteral("visible");

} // namespace BroadItem
