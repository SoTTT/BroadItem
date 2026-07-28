#pragma once

#include <broaditem/diagnostics/Diagnostic.h>

namespace BroadItem {

/// @brief 结构化诊断收集器接口。
///
/// 全库诊断经此接口单一口径投递；默认实现转发 Qt 消息系统（行为与旧版
/// 裸 qWarning/qCritical 兼容），可注入自定义实现以程序化收集。
class ErrorCollector {
public:
    virtual ~ErrorCollector() = default;

    /// @brief 接收一条诊断。
    /// @param diagnostic 结构化诊断内容。
    virtual void report(const Diagnostic& diagnostic) = 0;
};

/// @brief 默认收集器：格式化后转发 Qt 消息系统。
///
/// Error 级走 qCritical、Warning 级走 qWarning。这是全库唯一直接调用
/// Qt 消息输出的位置。
class DefaultErrorCollector : public ErrorCollector {
public:
    /// @brief 格式化诊断并转发 qCritical/qWarning。
    /// @param diagnostic 结构化诊断内容。
    void report(const Diagnostic& diagnostic) override;
};

} // namespace BroadItem
