/// @file Alignment.cpp
/// @brief 对齐枚举的字面量解析 —— 非法取值统一报告 BI-P-024 并回退默认值。

#include <broaditem/element/Alignment.h>
#include <broaditem/diagnostics/Diagnostics.h>

namespace BroadItem {

namespace {

/// @brief 报告枚举字面量取值非法（BI-P-024，恢复策略 Default：属性保持默认值）。
/// @param attrName 属性名。
/// @param value 非法取值。
/// @param validValues 合法取值列表（用于消息）。
void reportInvalidEnumLiteral(const QString& attrName, const QString& value, const char* validValues)
{
    Diagnostics::reportParse(ErrorCode::InvalidEnumLiteral,
                             QStringLiteral("Attribute %1 expects one of %2, got \"%3\"")
                                 .arg(attrName, QString::fromLatin1(validValues), value));
}

} // namespace

HAlign parseHAlign(const QString& value, HAlign fallback)
{
    if (value == QLatin1String("left"))   return HAlign::Left;
    if (value == QLatin1String("center")) return HAlign::Center;
    if (value == QLatin1String("right"))  return HAlign::Right;
    reportInvalidEnumLiteral(QStringLiteral("h-align"), value, "left|center|right");
    return fallback;
}

VAlign parseVAlign(const QString& value, VAlign fallback)
{
    if (value == QLatin1String("top"))    return VAlign::Top;
    if (value == QLatin1String("center")) return VAlign::Center;
    if (value == QLatin1String("bottom")) return VAlign::Bottom;
    reportInvalidEnumLiteral(QStringLiteral("v-align"), value, "top|center|bottom");
    return fallback;
}

MainAlign parseMainAlign(const QString& value, MainAlign fallback)
{
    if (value == QLatin1String("start"))         return MainAlign::Start;
    if (value == QLatin1String("center"))        return MainAlign::Center;
    if (value == QLatin1String("end"))           return MainAlign::End;
    if (value == QLatin1String("space-between")) return MainAlign::SpaceBetween;
    if (value == QLatin1String("space-around"))  return MainAlign::SpaceAround;
    if (value == QLatin1String("space-evenly"))  return MainAlign::SpaceEvenly;
    reportInvalidEnumLiteral(QStringLiteral("main-align"), value,
                             "start|center|end|space-between|space-around|space-evenly");
    return fallback;
}

CrossAlign parseCrossAlign(const QString& value, CrossAlign fallback, bool allowBaseline)
{
    if (value == QLatin1String("start"))   return CrossAlign::Start;
    if (value == QLatin1String("center"))  return CrossAlign::Center;
    if (value == QLatin1String("end"))     return CrossAlign::End;
    if (value == QLatin1String("stretch")) return CrossAlign::Stretch;
    if (allowBaseline && value == QLatin1String("baseline"))
        return CrossAlign::Baseline;
    reportInvalidEnumLiteral(QStringLiteral("cross-align"), value,
                             allowBaseline ? "start|center|end|stretch|baseline"
                                           : "start|center|end|stretch");
    return fallback;
}

} // namespace BroadItem
