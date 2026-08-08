#pragma once

#include <QVariant>
#include <QString>
#include <QtGlobal>

#include <memory>
#include <utility>

class QDomDocument;

namespace BroadItem {

/// @brief C++11 下的 std::make_unique 替代（std::make_unique 为 C++14 引入）。
///
/// 项目源码按 C++11 基线编写，所有 unique_ptr 构造统一经本函数，
/// 避免源码中散落裸 new。
///
/// @param args 转发给 T 构造函数的参数。
/// @return 新创建的 unique_ptr。
template <typename T, typename... Args>
std::unique_ptr<T> makeUnique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/// @brief 跨 Qt5/Qt6 的"值为 null"判定（<if> 存在性语义与空值移除语义的统一入口）。
///
/// Qt5：`QVariant::isNull()` 会把内含类型的 null 性传播上来（如 null QString
/// 使 QVariant 为 null）；Qt6：`isNull()` 仅对无效 QVariant 或空指针为 true，
/// `QVariant(QString())` 不再算 null。《设计.md》规定"null 值在 \<if\> 中视为
/// 不存在"、"setProperty 空值移除"依赖 Qt5 行为，故在此统一：
/// 两版下"无效 QVariant 或 null QString"均视为 null，其余类型按平台 isNull()。
///
/// @param v 待判定值。
/// @return 视为 null 返回 true。
inline bool variantIsNull(const QVariant& v)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!v.isValid())
        return true;
    if (v.userType() == QMetaType::QString)
        return v.toString().isNull();
    return v.isNull();
#else
    return v.isNull();
#endif
}

/// @brief QDomDocument::setContent 结果（版本无关形态）。
struct DomContentResult {
    bool ok = false;          ///< 解析成功。
    QString errorMessage;     ///< 错误消息（成功时为空）。
    int errorLine = 0;        ///< 错误行号。
    int errorColumn = 0;      ///< 错误列号。
};

/// @brief 跨 Qt5/Qt6 的 QDomDocument::setContent（命名空间处理开启）。
///
/// Qt6.8 起旧重载（bool 返回值 + 出参）废弃，改为 ParseOptions/ParseResult
/// 形态；差异集中在本兼容函数，调用点不出现版本分支。
///
/// @param doc 目标文档。
/// @param xml XML 文本。
/// @return 版本无关的解析结果。
DomContentResult domSetContent(QDomDocument& doc, const QString& xml);

} // namespace BroadItem
