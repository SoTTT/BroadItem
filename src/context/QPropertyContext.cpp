#include <broaditem/context/QPropertyContext.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <QDynamicPropertyChangeEvent>
#include <QDebug>

namespace BroadItem {

/// @brief 默认构造函数。创建没有目标对象的独立上下文。
QPropertyContext::QPropertyContext()
    : QObject(nullptr)
    , m_target(nullptr)
{
    QTimer::singleShot(0, this, [this] { ensureConnected(); });
}

/// @brief 构造一个代理目标 QObject 属性的上下文。
/// @param target The QObject whose Qt properties are exposed.
/// @param parent Optional QObject parent.
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
    QTimer::singleShot(0, this, [this] { ensureConnected(); });
}

/// @brief 析构函数。如果代理中，则从目标对象移除事件过滤器。
QPropertyContext::~QPropertyContext()
{
    if (isProxy() && m_target)
        m_target->removeEventFilter(this);
}

/// @brief 确保通知信号连接已建立（首次访问时调用一次）。
void QPropertyContext::ensureConnected()
{
    if (m_connected)
        return;
    m_connected = true;
    setupNotifyConnections();
}

/// @brief 连接目标对象的所有 Q_PROPERTY NOTIFY 信号到 onNotify()。
void QPropertyContext::setupNotifyConnections()
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
/// @param e The incoming event.
/// @return True if the event was handled.
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
/// @param obj The object the event originated from.
/// @param event The incoming event.
/// @return False to allow normal event processing to continue.
bool QPropertyContext::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::DynamicPropertyChange && obj == m_target) {
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
/// @param path The dotted/bracketed property path.
/// @param value The value to assign.
void QPropertyContext::setPropertyNested(const QString& path, const QVariant& value)
{
    int len = path.length();
    int dotPos = path.indexOf('.');
    int bracketPos = path.indexOf('[');
    int segEnd = len;
    if (dotPos >= 0 && bracketPos >= 0)
        segEnd = qMin(dotPos, bracketPos);
    else if (dotPos >= 0)
        segEnd = dotPos;
    else if (bracketPos >= 0)
        segEnd = bracketPos;

    QString firstKey = path.left(segEnd);
    if (firstKey.isEmpty()) {
        Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                   QStringLiteral("empty first key in path"));
        return;
    }

    ensureConnected();
    QObject* obj = m_target ? m_target : this;
    QByteArray firstKeyBa = firstKey.toUtf8();

    QVariant root = obj->property(firstKeyBa.constData());
    if (!root.isValid()) {
        Diagnostics::reportRuntime(ErrorCode::ObjectFirstKeyMissing, path,
                                   QStringLiteral("property %1 not found on object").arg(firstKey));
        return;
    }

    if (!setWalkInto(root, path, segEnd, value))
        return;

    if (obj->metaObject()->indexOfProperty(firstKeyBa.constData()) >= 0) {
        if (!obj->setProperty(firstKeyBa.constData(), root)) {
            Diagnostics::reportRuntime(ErrorCode::SetPropertyTypeMismatch, path,
                                       QStringLiteral("setProperty: type mismatch for %1").arg(firstKey));
        }
    } else {
        obj->setProperty(firstKeyBa.constData(), root);
    }
    // Notification is delivered automatically via Q_PROPERTY NOTIFY signal
    // (for declared properties) or QDynamicPropertyChangeEvent (for dynamic ones).
}

} // namespace BroadItem
