#include <broaditem/context/QPropertyContext.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <QDynamicPropertyChangeEvent>
#include <QMetaProperty>
#include <QTimer>

namespace BroadItem {

/// @brief 默认构造函数。创建没有目标对象的独立上下文。
QPropertyContext::QPropertyContext()
    : QObject(nullptr)
    , m_target(nullptr)
{
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
    QTimer::singleShot(0, this, [this] { ensureConnected(); });
}

/// @brief 析构函数。如果代理中，则从目标对象移除事件过滤器。
QPropertyContext::~QPropertyContext()
{
    if (isProxy() && m_target)
        m_target->removeEventFilter(this);
}

/// @brief 确保通知信号连接已建立（首次访问时调用一次）。
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
    int segEnd = segmentEnd(path, 0);

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
    // 变更通知自动投递：声明属性走 Q_PROPERTY NOTIFY 信号，
    // 动态属性走 QDynamicPropertyChangeEvent。
}

} // namespace BroadItem
