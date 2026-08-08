#pragma once

#include <QtGlobal>

namespace BroadItem {

/// @brief reactive 模块场景图元的 z 值层级约定。
///
/// 连接线与装饰器外框位于 z=1，锚点位于 z=2，
/// 保证锚点始终绘制在连接线与装饰器外框之上。
/// 本头为 src/reactive 内部实现细节，不对外导出。

/// @brief 连接线与装饰器外框的 z 值。
constexpr qreal kDecorationZValue = 1.0;

/// @brief 锚点的 z 值，高于连接线与装饰器外框。
constexpr qreal kAnchorZValue = 2.0;

} // namespace BroadItem
