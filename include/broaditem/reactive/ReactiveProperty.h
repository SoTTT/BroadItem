#pragma once

#include <QString>

namespace BroadItem {

/// @brief 响应式属性键常量，用于将常用属性名映射到 QGraphicsObject 的 getter/setter。
///
/// ReactiveBinding 使用这些常量识别需要特殊处理或频繁使用的属性（如 pos、scale 等）。
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
    /// @brief 宽度属性键。
    static const QString Width;
    /// @brief 高度属性键。
    static const QString Height;
};

inline const QString Property::Pos      = QStringLiteral("pos");
inline const QString Property::Scale    = QStringLiteral("scale");
inline const QString Property::Rotation = QStringLiteral("rotation");
inline const QString Property::Opacity  = QStringLiteral("opacity");
inline const QString Property::Visible  = QStringLiteral("visible");
inline const QString Property::Width    = QStringLiteral("width");
inline const QString Property::Height   = QStringLiteral("height");

} // namespace BroadItem
