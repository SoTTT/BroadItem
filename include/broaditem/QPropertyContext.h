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
 * 通知机制通过 NOTIFY 信号自动连接（构造后 QTimer::singleShot 延迟连接 + 各 API 同步兜底）
 * 和 QDynamicPropertyChangeEvent 拦截实现。
 */
class QPropertyContext : public QObject, public PropertyContext {
    Q_OBJECT
public:
    /// @brief 构造自托管 QPropertyContext。
    QPropertyContext();
    /// @brief 为指定的目标对象构造代理 QPropertyContext。
    explicit QPropertyContext(QObject* target, QObject* parent = nullptr);
    ~QPropertyContext() override;

    /// @brief 为 Q_PROPERTY 变更设置 NOTIFY 信号连接。
    void setupNotifyConnections();

    QVariant property(const QString& name) const override
    {
        if (name.isEmpty())
            return QVariant();
        const_cast<QPropertyContext*>(this)->ensureConnected();
        const QObject* obj = m_target ? m_target : this;
        if (!name.contains('.') && !name.contains('['))
            return obj->property(name.toUtf8().constData());
        return walkPathFromObject(obj, name);
    }

    bool hasProperty(const QString& name) const override
    {
        if (name.isEmpty())
            return false;
        const_cast<QPropertyContext*>(this)->ensureConnected();
        const QObject* obj = m_target ? m_target : this;
        if (!name.contains('.') && !name.contains('['))
            return obj->metaObject()->indexOfProperty(name.toUtf8().constData()) >= 0;
        return walkPathFromObject(obj, name).isValid();
    }

    void setProperty(const QString& name, const QVariant& value) override
    {
        if (!name.contains('.') && !name.contains('[')) {
            // Flat key — existing behavior
            ensureConnected();
            QObject* obj = m_target ? m_target : this;
            QByteArray nameBa = name.toUtf8();
            if (obj->metaObject()->indexOfProperty(nameBa.constData()) >= 0) {
                if (!obj->setProperty(nameBa.constData(), value)) {
                    qWarning() << "QPropertyContext::setProperty: type mismatch for" << name
                               << "(type:" << value.typeName() << ")";
                }
            } else {
                obj->setProperty(nameBa.constData(), value);
            }
            return;
        }
        // Nested path
        setPropertyNested(name, value);
    }

    bool event(QEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onNotify();

private:
    void ensureConnected();
    bool isProxy() const { return m_target != nullptr; }
    void commonSetup();

    void setPropertyNested(const QString& path, const QVariant& value);

    QObject* m_target = nullptr;  ///< Target QObject for proxy mode, nullptr in self-hosted mode.
    bool m_connected = false;     ///< Whether NOTIFY signal connections have been set up.
};

} // namespace BroadItem
