#pragma once

/// @file diagnostics_helpers.h
/// @brief 测试共享辅助：捕获型诊断收集器（原散落于各测试文件的重复定义）。

#include <broaditem/diagnostics/Diagnostics.h>

#include <QList>
#include <QMutex>
#include <QMutexLocker>

namespace BroadItem {
namespace TestHelpers {

/// @brief 捕获型收集器：记录全部诊断供断言（report 经互斥锁保护，可多线程并发投递；
///        count/find/list 读取须在投递结束后进行）。
class CaptureCollector : public ErrorCollector {
public:
    void report(const Diagnostic& d) override
    {
        QMutexLocker lock(&m_mutex);
        list.append(d);
    }

    QList<Diagnostic> list;  ///< 已捕获的诊断序列。

    /// @brief 统计指定错误码出现次数。
    int count(ErrorCode c) const
    {
        int n = 0;
        for (const auto& d : list)
            if (d.code == c)
                ++n;
        return n;
    }

    /// @brief 查找指定错误码的首条诊断（未命中返回 nullptr）。
    const Diagnostic* find(ErrorCode c) const
    {
        for (const auto& d : list)
            if (d.code == c)
                return &d;
        return nullptr;
    }

private:
    QMutex m_mutex;  ///< 保护 list 的互斥锁（多线程 parse/管线并发投递场景）。
};

} // namespace TestHelpers
} // namespace BroadItem
