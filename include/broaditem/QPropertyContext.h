#pragma once

#include "PropertyContext.h"
#include <QObject>
#include <QMetaProperty>
#include <QTimer>

namespace BroadItem {

/**
 * @brief 基于 Q_PROPERTY 的属性上下文，支持双模式。
 *
 * 自宿主模式（target = this）：
 *   监视自身声明的 Q_PROPERTY，通过 event() 拦截动态属性。
 *
 * 代理模式（target = QObject*）：
 *   监视目标对象的 Q_PROPERTY，通过 eventFilter 拦截动态属性。
 *
 * 通知机制通过 NOTIFY 信号自动连接（QTimer::singleShot + setProperty 同步兜底）
 * 和 QDynamicPropertyChangeEvent 拦截实现。
 */
class QPropertyContext : public QObject, public PropertyContext {
    Q_OBJECT
public:
    QPropertyContext();
    explicit QPropertyContext(QObject* target, QObject* parent = nullptr);
    ~QPropertyContext() override;

    void setupNotifyConnections();

    QVariant property(const QString& name) const override
    {
        if (name.isEmpty())
            return QVariant();
        const QObject* obj = m_target ? m_target : static_cast<const QPropertyContext*>(this);
        if (!name.contains('.') && !name.contains('['))
            return obj->property(name.toUtf8().constData());
        return walkPathFromObject(obj, name);
    }

    bool hasProperty(const QString& name) const override
    {
        if (name.isEmpty())
            return false;
        const QObject* obj = m_target ? m_target : static_cast<const QPropertyContext*>(this);
        if (!name.contains('.') && !name.contains('['))
            return obj->metaObject()->indexOfProperty(name.toUtf8().constData()) >= 0;
        return walkPathFromObject(obj, name).isValid();
    }

    void setProperty(const QString& name, const QVariant& value) override
    {
        ensureConnected();
        QObject* obj = m_target ? m_target : static_cast<QPropertyContext*>(this);
        obj->setProperty(name.toUtf8().constData(), value);
    }

    bool event(QEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onNotify();

private:
    void ensureConnected();
    bool isProxy() const { return m_target != nullptr; }
    void commonSetup();

    QObject* m_target = nullptr;
    bool m_connected = false;
};

} // namespace BroadItem
