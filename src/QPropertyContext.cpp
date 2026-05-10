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

} // namespace BroadItem
