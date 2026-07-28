#pragma once

#include <broaditem/diagnostics/ErrorCollector.h>
#include <QString>
#include <QStringList>
#include <memory>

namespace BroadItem {

class Element;

/// @brief 诊断报告入口与解析/运行时作用域。
///
/// 单线程假设（与库其余部分一致）：作用域栈为函数级静态对象，
/// 仅供 GUI 主线程使用。
namespace Diagnostics {

/// @brief 返回进程级收集器（默认为 DefaultErrorCollector）。
/// @return 当前进程级收集器。
std::shared_ptr<ErrorCollector> collector();

/// @brief 注入进程级收集器（nullptr 恢复默认）。
/// @param c 新收集器。
void setCollector(std::shared_ptr<ErrorCollector> c);

/// @brief 向当前解析会话压入元素路径段（无会话时无操作）。仅供 XmlLayoutParser。
/// @param segment 路径段（如 "text" 或 "column[2]"）。
void pushElementSegment(const QString& segment);

/// @brief 弹出当前解析会话的元素路径段（无会话时无操作）。仅供 XmlLayoutParser。
void popElementSegment();

/// @brief 解析期诊断报告。
///
/// 存在当前 ParseSession 时补充 file/elementPath，Abort 级错误自动置
/// 会话 sawAbort 标志；投递给会话本次收集器（若有）与进程级收集器。
/// 无会话时直投进程级收集器（Registry 等解析期外场景）。
///
/// @param code    错误码。
/// @param message 模板化消息。
/// @param file    无会话时的文件路径（有会话时忽略，取会话值）。
/// @param line    行号（仅 XML 语法错误）。
/// @param column  列号（仅 XML 语法错误）。
void reportParse(ErrorCode code, const QString& message,
                 const QString& file = QString(), int line = -1, int column = -1);

/// @brief 运行时诊断报告。
///
/// 存在当前 RuntimeScope 时按（模板 × 错误码 × 绑定路径）去重——同一组合
/// 只投递一次；无 scope（直接跑 LayoutEngine、写时错误等）不去重照报。
///
/// @param code        错误码。
/// @param bindingPath 触发诊断的绑定路径（去重键的一部分）。
/// @param message     模板化消息。
void reportRuntime(ErrorCode code, const QString& bindingPath, const QString& message);

/// @brief 解析会话（RAII），仅 XmlLayoutParser 在 parse 期间构造。
///
/// 持有本次调用收集器（可空）、文件路径、元素路径段栈与 sawAbort 标志；
/// 构造/析构自动维护会话栈，嵌套 parse 安全。
class ParseSession {
public:
    /// @brief 开启解析会话。
    /// @param file              布局文件路径（parseString 直接调用时为空）。
    /// @param overrideCollector 本次调用专用收集器（可空，叠加于进程级）。
    explicit ParseSession(const QString& file, ErrorCollector* overrideCollector);
    ~ParseSession();

    ParseSession(const ParseSession&) = delete;
    ParseSession& operator=(const ParseSession&) = delete;

    /// @brief 压入元素路径段（如 "text" 或 "column[2]"）。
    /// @param segment 路径段。
    void pushSegment(const QString& segment);

    /// @brief 弹出栈顶路径段。
    void popSegment();

    /// @brief 当前完整元素路径（如 root/column[2]/text）。
    /// @return 以 '/' 连接的路径。
    QString elementPath() const { return m_segments.join(QLatin1Char('/')); }

    /// @brief 会话期间是否发生过 Abort 级错误。
    /// @return 发生过返回 true（parse 应返回 nullptr）。
    bool sawAbort() const { return m_sawAbort; }

private:
    QString m_file;                     ///< 布局文件路径。
    ErrorCollector* m_override;         ///< 本次调用专用收集器（不持有）。
    QStringList m_segments;             ///< 元素路径段栈。
    bool m_sawAbort = false;            ///< Abort 级错误发生标志。

    friend void reportParse(ErrorCode, const QString&, const QString&, int, int);
};

/// @brief 运行时作用域（RAII），仅 Frame 在管线期间构造。
///
/// 持有模板根元素指针；reportRuntime 以其为去重作用域。
class RuntimeScope {
public:
    /// @brief 开启运行时作用域。
    /// @param templateRoot 模板根元素（去重集合宿主）。
    explicit RuntimeScope(const Element* templateRoot);
    ~RuntimeScope();

    RuntimeScope(const RuntimeScope&) = delete;
    RuntimeScope& operator=(const RuntimeScope&) = delete;

private:
    const Element* m_root;  ///< 模板根元素。

    friend void reportRuntime(ErrorCode, const QString&, const QString&);
};

} // namespace Diagnostics

} // namespace BroadItem
