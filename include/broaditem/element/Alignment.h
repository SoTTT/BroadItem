#pragma once

#include <QString>

namespace BroadItem {

/// @brief 水平对齐取值（text/cell 的 h-align 属性）。
enum class HAlign { Left, Center, Right };

/// @brief 垂直对齐取值（text/cell 的 v-align 属性）。
enum class VAlign { Top, Center, Bottom };

/// @brief 主轴对齐取值（row/column 的 main-align 属性）。
enum class MainAlign { Start, Center, End, SpaceBetween, SpaceAround, SpaceEvenly };

/// @brief 交叉轴对齐取值（row/column 的 cross-align 属性）。
///
/// Baseline 仅 row 合法（column 无基线语义，解析期按非法取值拒绝）。
enum class CrossAlign { Start, Center, End, Stretch, Baseline };

/// @brief 按水平对齐计算内容在可用区间内的起始位置。
/// @param start 区间起点。
/// @param avail 区间长度。
/// @param content 内容长度。
/// @return 对齐后的内容起始位置（Left = start，Center = 居中，Right = 末端对齐）。
inline double hAligned(double start, double avail, double content, HAlign align)
{
    switch (align) {
    case HAlign::Center: return start + (avail - content) / 2.0;
    case HAlign::Right:  return start + avail - content;
    case HAlign::Left:   break;
    }
    return start;
}

/// @brief 按垂直对齐计算内容在可用区间内的起始位置。
/// @param start 区间起点。
/// @param avail 区间长度。
/// @param content 内容长度。
/// @return 对齐后的内容起始位置（Top = start，Center = 居中，Bottom = 末端对齐）。
inline double vAligned(double start, double avail, double content, VAlign align)
{
    switch (align) {
    case VAlign::Center: return start + (avail - content) / 2.0;
    case VAlign::Bottom: return start + avail - content;
    case VAlign::Top:    break;
    }
    return start;
}

/// @brief 解析 h-align 字面量为枚举；非法取值报告 BI-P-024 并返回 fallback。
/// @param value 字面量取值。
/// @param fallback 非法取值时的回退值（传入成员当前默认值）。
/// @return 解析得到的枚举值。
HAlign parseHAlign(const QString& value, HAlign fallback);

/// @brief 解析 v-align 字面量为枚举；非法取值报告 BI-P-024 并返回 fallback。
/// @param value 字面量取值。
/// @param fallback 非法取值时的回退值（传入成员当前默认值）。
/// @return 解析得到的枚举值。
VAlign parseVAlign(const QString& value, VAlign fallback);

/// @brief 解析 main-align 字面量为枚举；非法取值报告 BI-P-024 并返回 fallback。
/// @param value 字面量取值。
/// @param fallback 非法取值时的回退值（传入成员当前默认值）。
/// @return 解析得到的枚举值。
MainAlign parseMainAlign(const QString& value, MainAlign fallback);

/// @brief 解析 cross-align 字面量为枚举；非法取值报告 BI-P-024 并返回 fallback。
/// @param value 字面量取值。
/// @param fallback 非法取值时的回退值（传入成员当前默认值）。
/// @param allowBaseline 是否接受 "baseline"（仅 row 传 true）。
/// @return 解析得到的枚举值。
CrossAlign parseCrossAlign(const QString& value, CrossAlign fallback, bool allowBaseline);

} // namespace BroadItem
