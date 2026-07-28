#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/element/Element.h>
#include <QDebug>

namespace BroadItem {

// ========== 码表（级别/恢复策略由码唯一决定，与 doc/设计.md「诊断」节一一对应） ==========

namespace {

struct CodeInfo {
    const char* str;
    Severity severity;
    Recovery recovery;
};

/// 码表：与 ErrorCode 枚举值按声明顺序一一对应。
const CodeInfo kCodeTable[] = {
    // ---- 解析期（BI-P-xxx） ----
    { "BI-P-001", Severity::Error,   Recovery::Abort   },  // FileOpenFailed
    { "BI-P-002", Severity::Error,   Recovery::Abort   },  // XmlSyntaxError
    { "BI-P-003", Severity::Error,   Recovery::Abort   },  // RootNotRoot
    { "BI-P-004", Severity::Error,   Recovery::Abort   },  // RootChildCount
    { "BI-P-005", Severity::Error,   Recovery::Abort   },  // UnknownElementTag
    { "BI-P-006", Severity::Warning, Recovery::Default },  // DeprecatedDecoratorTag
    { "BI-P-007", Severity::Error,   Recovery::Abort   },  // LeafWithChildren
    { "BI-P-008", Severity::Error,   Recovery::Abort   },  // GridNonCellChild
    { "BI-P-009", Severity::Error,   Recovery::Abort   },  // GridCellCountMismatch
    { "BI-P-010", Severity::Error,   Recovery::Abort   },  // IfMissingProp
    { "BI-P-011", Severity::Warning, Recovery::Default },  // UnknownAttribute
    { "BI-P-012", Severity::Warning, Recovery::Default },  // UnknownBindingAttribute
    { "BI-P-013", Severity::Warning, Recovery::Default },  // BindingNotResolved
    { "BI-P-014", Severity::Error,   Recovery::Default },  // MutexLiteralBinding
    { "BI-P-015", Severity::Error,   Recovery::Default },  // LiteralTypeMismatch
    { "BI-P-016", Severity::Error,   Recovery::Default },  // LiteralOutOfRange
    { "BI-P-017", Severity::Warning, Recovery::Default },  // GridTooManyChildren
    { "BI-P-018", Severity::Warning, Recovery::Default },  // AsWithoutOf
    { "BI-P-019", Severity::Warning, Recovery::Default },  // NotBindingIgnored
    { "BI-P-020", Severity::Warning, Recovery::Default },  // RegistryDirMissing
    { "BI-P-021", Severity::Warning, Recovery::Skip    },  // RegistryFileSkipped
    { "BI-P-022", Severity::Error,   Recovery::Abort   },  // RootChildDiscarded
    // ---- 运行时（BI-R-xxx） ----
    { "BI-R-001", Severity::Error,   Recovery::Default },  // ObjectFirstKeyMissing
    { "BI-R-002", Severity::Error,   Recovery::Default },  // NestedKeyMissing
    { "BI-R-003", Severity::Error,   Recovery::Default },  // TraverseTypeMismatch
    { "BI-R-004", Severity::Error,   Recovery::Default },  // IndexOutOfBounds
    { "BI-R-005", Severity::Error,   Recovery::Default },  // PathSyntaxError
    { "BI-R-006", Severity::Warning, Recovery::Default },  // SetPropertyTypeMismatch
    { "BI-R-007", Severity::Error,   Recovery::Default },  // ForMissingAs
    { "BI-R-008", Severity::Error,   Recovery::Default },  // ForDataNotList
    { "BI-R-009", Severity::Warning, Recovery::Skip    },  // ForNullItemSkipped
    { "BI-R-010", Severity::Warning, Recovery::Default },  // ImageLoadFailed
    { "BI-R-011", Severity::Error,   Recovery::Default },  // BoundValueTypeError
};

const CodeInfo& infoOf(ErrorCode code)
{
    return kCodeTable[static_cast<size_t>(code)];
}

} // namespace

QString codeToString(ErrorCode code)
{
    return QString::fromLatin1(infoOf(code).str);
}

Severity severityOf(ErrorCode code)
{
    return infoOf(code).severity;
}

Recovery recoveryOf(ErrorCode code)
{
    return infoOf(code).recovery;
}

// ========== 默认收集器 ==========

void DefaultErrorCollector::report(const Diagnostic& diagnostic)
{
    QString text = QStringLiteral("[%1] ").arg(codeToString(diagnostic.code));
    if (!diagnostic.file.isEmpty()) {
        text += diagnostic.file;
        if (diagnostic.line > 0)
            text += QStringLiteral(":%1:%2").arg(diagnostic.line).arg(diagnostic.column);
        text += QStringLiteral(": ");
    }
    if (!diagnostic.elementPath.isEmpty())
        text += QStringLiteral("<%1> ").arg(diagnostic.elementPath);
    if (!diagnostic.bindingPath.isEmpty())
        text += QStringLiteral("(%1) ").arg(diagnostic.bindingPath);
    text += diagnostic.message;

    if (severityOf(diagnostic.code) == Severity::Error)
        qCritical().noquote() << text;
    else
        qWarning().noquote() << text;
}

// ========== Diagnostics 命名空间 ==========

namespace Diagnostics {

namespace {

/// 进程级收集器（默认 DefaultErrorCollector）。
std::shared_ptr<ErrorCollector>& processCollector()
{
    static std::shared_ptr<ErrorCollector> c = std::make_shared<DefaultErrorCollector>();
    return c;
}

/// 会话栈（单线程假设）。
std::vector<ParseSession*>& sessionStack()
{
    static std::vector<ParseSession*> stack;
    return stack;
}

/// 运行时作用域栈（单线程假设）。
std::vector<const Element*>& runtimeStack()
{
    static std::vector<const Element*> stack;
    return stack;
}

} // namespace

std::shared_ptr<ErrorCollector> collector()
{
    return processCollector();
}

void setCollector(std::shared_ptr<ErrorCollector> c)
{
    processCollector() = c ? std::move(c) : std::make_shared<DefaultErrorCollector>();
}

void pushElementSegment(const QString& segment)
{
    if (!sessionStack().empty())
        sessionStack().back()->pushSegment(segment);
}

void popElementSegment()
{
    if (!sessionStack().empty())
        sessionStack().back()->popSegment();
}

void reportParse(ErrorCode code, const QString& message,
                 const QString& file, int line, int column)
{
    Diagnostic d;
    d.code = code;
    d.message = message;
    d.line = line;
    d.column = column;

    ParseSession* session = sessionStack().empty() ? nullptr : sessionStack().back();
    if (session) {
        d.file = session->m_file;
        d.elementPath = session->elementPath();
        if (recoveryOf(code) == Recovery::Abort)
            session->m_sawAbort = true;
        if (session->m_override)
            session->m_override->report(d);
    } else {
        d.file = file;
    }
    processCollector()->report(d);
}

void reportRuntime(ErrorCode code, const QString& bindingPath, const QString& message)
{
    // 模板级去重：同一（模板 × 错误码 × 绑定路径）只投递一次
    if (!runtimeStack().empty()) {
        const Element* root = runtimeStack().back();
        const QString key = codeToString(code) + QLatin1Char('|') + bindingPath;
        if (root && !root->markDiagnosticReported(key))
            return;
    }

    Diagnostic d;
    d.code = code;
    d.bindingPath = bindingPath;
    d.message = message;
    processCollector()->report(d);
}

// ---- ParseSession ----

ParseSession::ParseSession(const QString& file, ErrorCollector* overrideCollector)
    : m_file(file)
    , m_override(overrideCollector)
{
    sessionStack().push_back(this);
}

ParseSession::~ParseSession()
{
    sessionStack().pop_back();
}

void ParseSession::pushSegment(const QString& segment)
{
    m_segments.append(segment);
}

void ParseSession::popSegment()
{
    if (!m_segments.isEmpty())
        m_segments.removeLast();
}

// ---- RuntimeScope ----

RuntimeScope::RuntimeScope(const Element* templateRoot)
    : m_root(templateRoot)
{
    runtimeStack().push_back(m_root);
}

RuntimeScope::~RuntimeScope()
{
    runtimeStack().pop_back();
}

} // namespace Diagnostics

} // namespace BroadItem
