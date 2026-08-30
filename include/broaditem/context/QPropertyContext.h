#pragma once

#include <broaditem/context/PropertyContext.h>
#include <QObject>

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
 *
 * @warning 共存风险：若对同一 QObject 同时使用 QPropertyContext（proxy 模式）
 * 与 ReactiveBinding，该 QObject 的 Q_PROPERTY NOTIFY 信号会被两套系统同时连接。
 * 对 visible/opacity 等两套系统均可绑定的属性，属性变更可能触发级联更新。
 * 当前版本两套系统默认隔离（BroadItem 默认使用 MapPropertyContext），
 * 此风险仅在显式向 BroadItem 传入 QPropertyContext 时出现。
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
    void setupNotifyConnections() const;

    QVariant property(const QString& name) const override
    {
        if (name.isEmpty())
            return QVariant();
        ensureConnected();
        const QObject* obj = m_target ? m_target : this;
        if (!name.contains('.') && !name.contains('['))
            return obj->property(name.toUtf8().constData());
        return walkPathFromObject(obj, name);
    }

    bool hasProperty(const QString& name) const override
    {
        if (name.isEmpty())
            return false;
        ensureConnected();
        const QObject* obj = m_target ? m_target : this;
        if (!name.contains('.') && !name.contains('['))
            return obj->metaObject()->indexOfProperty(name.toUtf8().constData()) >= 0;
        return walkPathFromObject(obj, name).isValid();
    }

    void setProperty(const QString& name, const QVariant& value) override
    {
        if (!name.contains('.') && !name.contains('[')) {
            // 扁平键——保持既有行为
            ensureConnected();
            QObject* obj = m_target ? m_target : this;
            QByteArray nameBa = name.toUtf8();
            if (obj->metaObject()->indexOfProperty(nameBa.constData()) >= 0) {
                if (!obj->setProperty(nameBa.constData(), value)) {
                    Diagnostics::reportRuntime(ErrorCode::SetPropertyTypeMismatch, name,
                                               QStringLiteral("setProperty: type mismatch for %1 (type: %2)")
                                                   .arg(name, QLatin1String(value.typeName())));
                }
            } else {
                obj->setProperty(nameBa.constData(), value);
            }
            return;
        }
        // 嵌套路径
        setPropertyNested(name, value);
    }

    bool event(QEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onNotify();

private:
    void ensureConnected() const;
    bool isProxy() const { return m_target != nullptr; }

    void setPropertyNested(const QString& path, const QVariant& value);

    QObject* m_target = nullptr;   ///< 代理模式的目标 QObject；自宿主模式为 nullptr。
    mutable bool m_connected = false;  ///< NOTIFY 信号连接是否已建立（mutable：const 读取路径惰性连接）。
};

} // namespace BroadItem
