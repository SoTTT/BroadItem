#pragma once

/// @file frame_helpers.h
/// @brief 测试共享辅助：从 XML 布局片段直接构造 Frame。
///        经 QBuffer 走 Frame::fromDevice，替代原五处"QTemporaryFile 落盘再
///        Frame::fromFile"的同构样板——临时文件、flush 时序与生命周期问题一并消除。

#include <broaditem/core/Frame.h>
#include <broaditem/context/PropertyContext.h>

#include <QBuffer>
#include <QString>
#include <memory>

#include "binding_helpers.h"

namespace BroadItem {
namespace TestHelpers {

/// @brief 从布局片段构造 Frame：自动包裹 <root xmlns:b> 头尾，经 QBuffer 走 fromDevice。
///
/// data/buffer 只需活到 fromDevice 返回（构造期完成全部读取与首次布局），
/// 局部变量即安全——与旧临时文件写法"保活覆盖 Frame 构造"的误解不同。
///
/// @param inner 根元素唯一子元素的 XML 片段（可引用 b: 前缀）。
/// @param ctx   属性上下文（为 nullptr 时 Frame 自动创建默认上下文）。
/// @return 解析并布局完成的 Frame。
inline std::unique_ptr<Frame> frameFromXmlString(const QString& inner,
                                                 const std::shared_ptr<PropertyContext>& ctx = nullptr)
{
    QByteArray data = wrap(inner).toUtf8();
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);
    return Frame::fromDevice(&buffer, ctx);
}

} // namespace TestHelpers
} // namespace BroadItem
