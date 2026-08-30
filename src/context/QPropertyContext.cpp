#include <broaditem/context/QPropertyContext.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/expression/Expression.h>
#include <QDynamicPropertyChangeEvent>
#include <QMetaProperty>
#include <QTimer>

namespace BroadItem {

/// @brief 默认构造函数。创建没有目标对象的独立上下文。
QPropertyContext::QPropertyContext()
    : QObject(nullptr)
    , m_target(nullptr)
{
    // 延迟到事件循环下一拍连接：自宿主模式构造期派生类 Q_PROPERTY 尚未进入
    // 元对象（三路径惰性连接的完整约束见 ensureConnected 注释）
    QTimer::singleShot(0, this, [this] { ensureConnected(); });
}

/// @brief 构造一个代理目标 QObject 属性的上下文。
/// @param target 其 Qt 属性被暴露的 QObject。
/// @param parent 可选的 QObject 父对象。
QPropertyContext::QPropertyContext(QObject* target, QObject* parent)
    : QObject(parent)
    , m_target(target)
{
    if (target) {
        target->installEventFilter(this);
        connect(target, &QObject::destroyed, this, [this] {
            m_target = nullptr;
        });
    }
    // 同默认构造：统一走定时器延迟 + 各 API/eventFilter 同步兜底（见 ensureConnected 注释）
    QTimer::singleShot(0, this, [this] { ensureConnected(); });
}

/// @brief 析构函数。如果代理中，则从目标对象移除事件过滤器。
QPropertyContext::~QPropertyContext()
{
    if (isProxy() && m_target)
        m_target->removeEventFilter(this);
}

/// @brief 确保通知信号连接已建立（首次访问时调用一次）。
///
/// 一次性惰性初始化的唯一汇合点。三条触发路径各补一个别的路径盖不住的场景，
/// 均非冗余（土法编程审阅 #7 结论）：
/// - 构造函数只排 QTimer::singleShot(0) 而非构造即连：自宿主模式下被连接的是
///   this->metaObject()，基类构造期派生类的 Q_PROPERTY 尚未进入元对象，
///   构造即连会静默漏掉全部派生属性；推迟到事件循环下一拍时对象已完整构造。
/// - 各 API 入口（property/hasProperty/setProperty/setPropertyNested）同步兜底：
///   定时器依赖事件循环——调用方不跑事件循环（headless 直调）或在循环启动前
///   读写时，连接永不建立，变更通知静默丢失。
/// - eventFilter 同步兜底：纯监听场景（目标动态属性不经过本上下文 API 而变化）
///   下，DynamicPropertyChange 事件可能先于 singleShot(0) 到达。
/// m_connected 守卫保证三条路径叠加仍只连接一次。
void QPropertyContext::ensureConnected() const
{
    if (m_connected)
        return;
    m_connected = true;
    setupNotifyConnections();
}

/// @brief 连接目标对象的所有 Q_PROPERTY NOTIFY 信号到 onNotify()。
void QPropertyContext::setupNotifyConnections() const
{
    const QObject* obj = m_target ? m_target : this;
    const QMetaObject* mo = obj->metaObject();
    int slotIdx = metaObject()->indexOfSlot("onNotify()");

    for (int i = mo->propertyOffset(); i < mo->propertyCount(); i++) {
        QMetaProperty prop = mo->property(i);
        if (prop.hasNotifySignal()) {
            QMetaObject::connect(obj, prop.notifySignalIndex(),
                                 this, slotIdx,
                                 Qt::DirectConnection, nullptr);
        }
    }
}

/// @brief 当目标属性的 NOTIFY 信号触发时调用的槽函数。分发到 notifyChanged()。
void QPropertyContext::onNotify()
{
    const QObject* obj = m_target ? m_target : this;
    const QMetaObject* mo = obj->metaObject();
    int sigIdx = senderSignalIndex();

    for (int i = mo->propertyOffset(); i < mo->propertyCount(); i++) {
        QMetaProperty prop = mo->property(i);
        if (prop.hasNotifySignal() && prop.notifySignalIndex() == sigIdx) {
            notifyChanged(prop.name(), prop.read(obj));
            return;
        }
    }
}

/// @brief 处理独立（非代理）情况下的 DynamicPropertyChange 事件。
/// @param e 传入的事件。
/// @return 事件已处理时返回 true。
bool QPropertyContext::event(QEvent* e)
{
    if (e->type() == QEvent::DynamicPropertyChange) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto* de = static_cast<QDynamicPropertyChangeEvent*>(e);
        QString propName = de->propertyName();
        QVariant value = QObject::property(propName.toUtf8().constData());
        notifyChanged(propName, value);
        return true;
    }
    return QObject::event(e);
}

/// @brief 安装在目标对象上的事件过滤器，用于捕获 DynamicPropertyChange 事件。
/// @param obj 事件来源对象。
/// @param event 传入的事件。
/// @return 恒返回 false，让正常的事件处理继续。
bool QPropertyContext::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::DynamicPropertyChange && obj == m_target) {
        // 纯监听场景同步兜底：动态属性事件可能先于 singleShot(0) 到达（见 ensureConnected 注释）
        ensureConnected();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto* de = static_cast<QDynamicPropertyChangeEvent*>(event);
        QString propName = de->propertyName();
        QVariant value = obj->property(propName.toUtf8().constData());
        notifyChanged(propName, value);
        return false;
    }
    return QObject::eventFilter(obj, event);
}

/// @brief 使用点/括号表示法设置嵌套属性值（如 "obj.field[0].prop"）。
/// @param path 点号/括号形式的属性路径。
/// @param value 待设置的值。
void QPropertyContext::setPropertyNested(const QString& path, const QVariant& value)
{
    const Expression expr(path);
    if (!expr.isValid()) {
        Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                   QStringLiteral("invalid path syntax"));
        return;
    }
    const auto& segs = expr.segments();
    // 语法保证首段必为键分段。
    const QString& firstKey = segs.front().key();

    // 同步兜底：调用方可能不跑事件循环或在循环启动前写入，定时器永不触发
    //（见 ensureConnected 注释）
    ensureConnected();
    QObject* obj = m_target ? m_target : this;
    QByteArray firstKeyBa = firstKey.toUtf8();

    QVariant root = obj->property(firstKeyBa.constData());
    if (!root.isValid()) {
        Diagnostics::reportRuntime(ErrorCode::ObjectFirstKeyMissing, path,
                                   QStringLiteral("property %1 not found on object").arg(firstKey));
        return;
    }

    if (!setWalkSegments(root, segs, 1, path, value))
        return;

    if (obj->metaObject()->indexOfProperty(firstKeyBa.constData()) >= 0) {
        if (!obj->setProperty(firstKeyBa.constData(), root)) {
            Diagnostics::reportRuntime(ErrorCode::SetPropertyTypeMismatch, path,
                                       QStringLiteral("setProperty: type mismatch for %1").arg(firstKey));
        }
    } else {
        obj->setProperty(firstKeyBa.constData(), root);
    }
    // 变更通知自动投递：声明属性走 Q_PROPERTY NOTIFY 信号，
    // 动态属性走 QDynamicPropertyChangeEvent。
}

} // namespace BroadItem
