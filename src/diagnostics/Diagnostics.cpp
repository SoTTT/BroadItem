#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/element/Element.h>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <iterator>

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
    { "BI-P-014", Severity::Error,   Recovery::Default },  // MutexLiteralBinding
    { "BI-P-015", Severity::Error,   Recovery::Default },  // LiteralTypeMismatch
    { "BI-P-016", Severity::Error,   Recovery::Default },  // LiteralOutOfRange
    { "BI-P-018", Severity::Warning, Recovery::Default },  // AsWithoutOf
    { "BI-P-019", Severity::Warning, Recovery::Default },  // NotBindingIgnored
    { "BI-P-020", Severity::Warning, Recovery::Default },  // RegistryDirMissing
    { "BI-P-021", Severity::Warning, Recovery::Skip    },  // RegistryFileSkipped
    { "BI-P-022", Severity::Error,   Recovery::Abort   },  // RootChildDiscarded
    { "BI-P-023", Severity::Warning, Recovery::Default },  // BindingNotSupported
    { "BI-P-024", Severity::Error,   Recovery::Default },  // InvalidEnumLiteral
    { "BI-P-025", Severity::Warning, Recovery::Default },  // RegistryDuplicateId
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

// 码表与 ErrorCode 枚举按声明顺序一一对应：新增/插入枚举值时必须同步维护码表，
// 否则此断言编译失败，防止错位导致静默错配或越界。
static_assert(sizeof(kCodeTable) / sizeof(kCodeTable[0]) == static_cast<size_t>(ErrorCode::BoundValueTypeError) + 1,
              "kCodeTable 与 ErrorCode 枚举项数不一致");

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
        if (diagnostic.line)
            text += QStringLiteral(":%1:%2").arg(*diagnostic.line).arg(*diagnostic.column);
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

uint qHash(const DiagnosticKey& key)
{
    return ::qHash(key.bindingPath) ^ ::qHash(static_cast<int>(key.code));
}

namespace Diagnostics {

namespace {

/// 进程级收集器（默认 DefaultErrorCollector）。读写均须持 collectorMutex()。
std::shared_ptr<ErrorCollector>& processCollector()
{
    static std::shared_ptr<ErrorCollector> c = std::make_shared<DefaultErrorCollector>();
    return c;
}

/// 进程级收集器的互斥锁。注意：持锁期间不得调用 collector->report()
/// （用户收集器可能回调 Diagnostics 造成重入死锁）——锁内拷贝快照，锁外调用。
QMutex& collectorMutex()
{
    static QMutex m;
    return m;
}

/// 会话栈（thread_local：栈语义跨线程必串线，每线程各一条）。
std::vector<ParseSession*>& sessionStack()
{
    thread_local std::vector<ParseSession*> stack;
    return stack;
}

/// 运行时作用域栈（thread_local，同上）。
std::vector<QSet<DiagnosticKey>*>& runtimeStack()
{
    thread_local std::vector<QSet<DiagnosticKey>*> stack;
    return stack;
}

} // namespace

std::shared_ptr<ErrorCollector> collector()
{
    QMutexLocker lock(&collectorMutex());
    return processCollector();
}

void setCollector(std::shared_ptr<ErrorCollector> c)
{
    QMutexLocker lock(&collectorMutex());
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
                 const QString& file, const Optional<int>& line, const Optional<int>& column)
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
    collector()->report(d);
}

void reportRuntime(ErrorCode code, const QString& bindingPath, const QString& message)
{
    // 去重：同一（错误码 × 绑定路径）在当前 scope 的状态集合中只投递一次
    // （Frame 管线安装 scope，窗口为 Frame 实例生命周期；无 scope 照报）
    if (!runtimeStack().empty()) {
        QSet<DiagnosticKey>* state = runtimeStack().back();
        if (state) {
            const DiagnosticKey key{code, bindingPath};
            if (state->contains(key))
                return;
            state->insert(key);
        }
    }

    Diagnostic d;
    d.code = code;
    d.bindingPath = bindingPath;
    d.message = message;
    collector()->report(d);
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

RuntimeScope::RuntimeScope(QSet<DiagnosticKey>* dedupState)
    : m_state(dedupState)
{
    runtimeStack().push_back(m_state);
}

RuntimeScope::~RuntimeScope()
{
    runtimeStack().pop_back();
}

} // namespace Diagnostics

} // namespace BroadItem
