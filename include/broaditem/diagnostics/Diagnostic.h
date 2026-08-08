#pragma once

#include <QString>
#include <broaditem/compat/Optional.h>

namespace BroadItem {

/// @brief 诊断错误码（码表即规格，见 doc/设计.md「诊断」节）。
///
/// 级别与恢复策略由码唯一决定（severityOf/recoveryOf），调用点不可指定。
enum class ErrorCode {
    // ---- 解析期（BI-P-xxx） ----
    FileOpenFailed,          ///< BI-P-001 布局文件无法打开/读取
    XmlSyntaxError,          ///< BI-P-002 XML 语法错误（携 line/column）
    RootNotRoot,             ///< BI-P-003 根元素非 <root>
    RootChildCount,          ///< BI-P-004 <root> 子元素数 ≠ 1
    UnknownElementTag,       ///< BI-P-005 未知元素标签
    DeprecatedDecoratorTag,  ///< BI-P-006 deprecated 装饰器标签（<margin> 等）
    LeafWithChildren,        ///< BI-P-007 不可含子元素的部件含子元素
    GridNonCellChild,        ///< BI-P-008 grid 直接子元素非 <cell>
    GridCellCountMismatch,   ///< BI-P-009 grid cell 数 ≠ columns×rows
    IfMissingProp,           ///< BI-P-010 <if> 缺 b:prop
    UnknownAttribute,        ///< BI-P-011 未知属性
    UnknownBindingAttribute, ///< BI-P-012 未知绑定属性
    MutexLiteralBinding,     ///< BI-P-014 字面量与 b: 绑定互斥
    LiteralTypeMismatch,     ///< BI-P-015 字面量类型错误（数字/整数/布尔校验失败）
    LiteralOutOfRange,       ///< BI-P-016 字面量数值越界（font-size≤0、columns/rows≤0 等）
    AsWithoutOf,             ///< BI-P-018 b:as 无 b:of
    NotBindingIgnored,       ///< BI-P-019 <if> 的 b:not 绑定
    RegistryDirMissing,      ///< BI-P-020 Registry 批量加载：目录不存在
    RegistryFileSkipped,     ///< BI-P-021 Registry 批量加载：单文件解析失败
    RootChildDiscarded,      ///< BI-P-022 根唯一子元素降级为空（如 deprecated 装饰器），无可用内容
    BindingNotSupported,     ///< BI-P-023 布局策略属性不参与绑定（解析期拒绝，不注册）
    InvalidEnumLiteral,      ///< BI-P-024 枚举属性取值非法（h-align/v-align/main-align/cross-align）
    // ---- 运行时（BI-R-xxx） ----
    ObjectFirstKeyMissing,   ///< BI-R-001 绑定路径首段属性在 QObject 上不存在
    NestedKeyMissing,        ///< BI-R-002 路径中段键在 map 中不存在
    TraverseTypeMismatch,    ///< BI-R-003 路径遍历类型不符
    IndexOutOfBounds,        ///< BI-R-004 数组下标非法/越界
    PathSyntaxError,         ///< BI-R-005 路径语法错误
    SetPropertyTypeMismatch, ///< BI-R-006 setProperty 类型不匹配
    ForMissingAs,            ///< BI-R-007 <for> 缺 b:as
    ForDataNotList,          ///< BI-R-008 <for> 数据源非列表
    ForNullItemSkipped,      ///< BI-R-009 <for> 列表含 null 项
    ImageLoadFailed,         ///< BI-R-010 <image> 加载失败
    BoundValueTypeError,     ///< BI-R-011 绑定值类型错误
};

/// @brief 诊断严重级别。
enum class Severity { Error, Warning };

/// @brief 诊断恢复策略。
enum class Recovery { Abort, Skip, Default };

/// @brief 返回错误码的稳定字符串表示（如 "BI-P-005"）。
/// @param code 错误码。
/// @return 字符串码，可检索、可写入文档。
QString codeToString(ErrorCode code);

/// @brief 返回错误码绑定的严重级别。
/// @param code 错误码。
/// @return 级别（Error / Warning）。
Severity severityOf(ErrorCode code);

/// @brief 返回错误码绑定的恢复策略。
/// @param code 错误码。
/// @return 恢复策略（Abort / Skip / Default）。
Recovery recoveryOf(ErrorCode code);

/// @brief 一条结构化诊断。
struct Diagnostic {
    ErrorCode code;       ///< 错误码（级别/恢复策略由码决定）。
    QString file;         ///< 布局文件路径（解析期；无文件概念时为空）。
    Optional<int> line;   ///< 行号（仅 XML 语法错误可得）。
    Optional<int> column; ///< 列号（仅 XML 语法错误可得）。
    QString elementPath;  ///< 元素路径（解析期语义错误，如 root/column[2]/text）。
    QString bindingPath;  ///< 绑定路径（运行时错误）。
    QString message;      ///< 英文模板化消息。
};

} // namespace BroadItem
