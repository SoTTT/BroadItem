#pragma once

#include <broaditem/element/BoxModel.h>
#include <broaditem/compat/Optional.h>

namespace BroadItem {

/// @brief 实例层样式快照：物化时从模板成员 + PropertyContext 求值得到的样式集合。
///
/// 物化（materialize）阶段对模板层 Element 的绑定属性求值，结果固化为本快照，
/// 随 Node 进入 measure/layout/render 三阶段流水线使用。物化完成后快照只读，
/// 三阶段不再回查模板或上下文，保证模板不可变与实例状态分离。
///
/// @note 取舍说明：core 层直接引用 element/BoxModel.h，因为 Margin/Border/
/// Background/Padding 是纯数据结构、不依赖 element 层其他类型；为避免 core 层
/// 重复定义盒模型结构，选择直接复用而非拷贝定义。
struct ResolvedStyle {
    Margin margin;          ///< 求值后的外边距。
    Border border;          ///< 求值后的边框。
    Background background;  ///< 求值后的背景。
    Padding padding;        ///< 求值后的内边距。
    Optional<double> width;     ///< 求值后的指定宽度；空表示未指定（与 SizedElement 语义一致）。
    Optional<double> height;    ///< 求值后的指定高度；空表示未指定（与 SizedElement 语义一致）。
};

} // namespace BroadItem
