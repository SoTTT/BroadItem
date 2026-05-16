#include "broaditem/QPropertyContext.h"
#include <QDynamicPropertyChangeEvent>
#include <QDebug>

namespace BroadItem {

QPropertyContext::QPropertyContext()
    : QObject(nullptr)
    , m_target(nullptr)
{
    QTimer::singleShot(0, this, [this] { ensureConnected(); });
}

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

QPropertyContext::~QPropertyContext()
{
    if (isProxy() && m_target)
        m_target->removeEventFilter(this);
}

void QPropertyContext::ensureConnected()
{
    if (m_connected)
        return;
    m_connected = true;
    setupNotifyConnections();
}

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
        qCritical() << "QPropertyContext: empty first key in path" << path;
        return;
    }

    ensureConnected();
    QObject* obj = m_target ? m_target : this;
    QByteArray firstKeyBa = firstKey.toUtf8();

    QVariant root = obj->property(firstKeyBa.constData());
    if (!root.isValid()) {
        qCritical() << "QPropertyContext: property" << firstKey
                    << "not found on object (path:" << path << ")";
        return;
    }

    if (!setWalkInto(root, path, segEnd, value))
        return;

    if (obj->metaObject()->indexOfProperty(firstKeyBa.constData()) >= 0) {
        if (!obj->setProperty(firstKeyBa.constData(), root)) {
            qWarning() << "QPropertyContext::setProperty: type mismatch for" << firstKey
                       << "(nested path:" << path << ")";
        }
    } else {
        obj->setProperty(firstKeyBa.constData(), root);
    }
    // Notification is delivered automatically via Q_PROPERTY NOTIFY signal
    // (for declared properties) or QDynamicPropertyChangeEvent (for dynamic ones).
}

} // namespace BroadItem
