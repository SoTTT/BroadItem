#include <broaditem/compat/QtCompat.h>
#include <QDomDocument>

namespace BroadItem {

/// @brief 跨版本 setContent 实现：Qt6 走 ParseOptions/ParseResult 新重载，
/// Qt5 走旧的 bool + 出参重载（Qt6.8 起废弃）。
DomContentResult domSetContent(QDomDocument& doc, const QString& xml)
{
    DomContentResult r;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto result = doc.setContent(xml, QDomDocument::ParseOption::UseNamespaceProcessing);
    r.ok = static_cast<bool>(result);
    r.errorMessage = result.errorMessage;
    r.errorLine = static_cast<int>(result.errorLine);
    r.errorColumn = static_cast<int>(result.errorColumn);
#else
    r.ok = doc.setContent(xml, true, &r.errorMessage, &r.errorLine, &r.errorColumn);
#endif
    return r;
}

} // namespace BroadItem
