#pragma once

#include <QObject>
#include <QMetaObject>
#include <QVariant>
#include <QVector>
#include <functional>

namespace BroadItem {

/// @brief 响应式绑定类，将源 QObject 的属性变化单向同步到目标 QObject。
///
/// ReactiveBinding 通过 QMetaProperty::notifySignal() 自动发现并连接源属性的 NOTIFY 信号，
/// 当源属性发生变化时读取源值、应用可选的变换函数，然后通过 QObject::property()/setProperty()
/// 写入目标对象的对应属性。支持任何具备 NOTIFY 信号的 QObject 属性，以及特殊的 pos 属性
///（无 NOTIFY，改为连接 xChanged()/yChanged() 两个信号）。
///
/// 使用静态工厂方法 create() 构造，通过 destroy() 销毁。
/// 生命周期：监听源和目标的 destroyed() 信号以自动失效。
/// 循环检测：如果 evaluate() 重入自身则自动禁用并警告。
class ReactiveBinding : public QObject {
    Q_OBJECT
public:
    /// @brief 属性值变换函数类型，接收源值，返回变换后的值。
    using Transform = std::function<QVariant(const QVariant&)>;

    /// @brief 静态工厂：创建响应式绑定并返回裸指针。
    ///
    /// 校验：源和目标非空、属性名在对应对象上存在且可写、有 NOTIFY 信号或为 pos。
    /// 校验失败时输出 qWarning 并返回 nullptr。
    /// 如果源和目标相同且属性相同（自绑定），输出 qWarning 并返回 nullptr。
    /// @param source 源 QObject 指针。
    /// @param sourceProperty 源属性名（如 "pos"、"scale"、"rotation"、"opacity"、"visible"）。
    /// @param target 目标 QObject 指针。
    /// @param targetProperty 目标属性名。
    /// @param transform 可选的变换函数，默认为 nullptr（直通）。
    /// @return ReactiveBinding* 新绑定实例，失败时返回 nullptr。
    static ReactiveBinding* create(QObject* source,
                                   const QString& sourceProperty,
                                   QObject* target,
                                   const QString& targetProperty,
                                   Transform transform = nullptr);

    /// @brief 创建只观察源属性变化的绑定，不写入目标对象。
    ///
    /// 适用于需要响应属性变化但不需要同步到另一个属性的场景。回调函数接收源值，
    /// 其返回值会被忽略。
    /// @param source 源 QObject 指针。
    /// @param sourceProperty 源属性名。
    /// @param callback 属性变化时的回调函数。
    /// @param parent 父 QObject。
    /// @return ReactiveBinding* 新绑定实例；参数无效时返回 nullptr。
    static ReactiveBinding* createObserver(QObject* source,
                                           const QString& sourceProperty,
                                           Transform callback,
                                           QObject* parent = nullptr);

    /// @brief 销毁绑定，断开所有连接并标记为无效。
    void destroy();

    /// @brief 启用或禁用绑定。
    /// @param enabled true 启用，false 禁用。
    void setEnabled(bool enabled);

    /// @brief 查询绑定是否启用。
    /// @return true 表示绑定已启用。
    [[nodiscard]] bool isEnabled() const;

    /// @brief 手动触发一次属性评估：读取源属性，变换，非观察者模式下写入目标。
    ///
    /// 如果绑定已禁用或 m_evaluating 标志为 true（重入），则跳过执行。
    void evaluate();

    /// @brief 获取源对象。
    /// @return 源 QObject 指针，可能为 nullptr（已销毁）。
    [[nodiscard]] QObject* source() const;

    /// @brief 获取目标对象。
    /// @return 目标 QObject 指针，可能为 nullptr（已销毁）。
    [[nodiscard]] QObject* target() const;

    /// @brief 获取源属性名。
    /// @return 源属性名字符串。
    [[nodiscard]] QString sourceProperty() const;

    /// @brief 获取目标属性名。
    /// @return 目标属性名字符串。
    [[nodiscard]] QString targetProperty() const;

private slots:
    /// @brief 源属性变化时调用 evaluate()。
    void onSourceChanged();

private:
    /// @brief 私有构造，通过 create() 工厂创建。
    ReactiveBinding(QObject* source,
                    QString sourceProperty,
                    QObject* target,
                    QString targetProperty,
                    Transform transform,
                    QObject* parent = nullptr);

    /// @brief 连接源对象的对应属性变化信号。
    ///
    /// 对 scale/rotation/opacity/visible：通过 QMetaProperty::notifySignal() 获取信号方法并连接。
    /// 对 pos：连接 xChanged() 和 yChanged() 两个信号。
    void connectSourceSignal();

    /// @brief 从 QObject 读取属性值。
    static QVariant readProperty(const QObject* obj, const QString& prop);

    /// @brief 向 QObject 写入属性值。
    static void writeProperty(QObject* obj, const QString& prop, const QVariant& value);

    /// @brief 验证属性名在指定对象上有效。
    ///
    /// 条件：属性存在于 metaObject 中、可写、且有 NOTIFY 信号或为 Property::Pos。
    /// @param obj 要检查的 QObject。
    /// @param prop 属性名。
    /// @return true 表示属性有效。
    static bool isValidProperty(const QObject* obj, const QString& prop);

    QObject* m_source;                            ///< 源对象。
    QObject* m_target;                            ///< 目标对象。
    QString m_sourceProperty;                     ///< 源属性名。
    QString m_targetProperty;                     ///< 目标属性名。
    Transform m_transform;                        ///< 可选的变换函数。
    bool m_enabled;                               ///< 绑定是否启用。
    bool m_evaluating;                            ///< 循环检测标志，防止 evaluate() 重入。

    QVector<QMetaObject::Connection> m_signalConnections;       ///< 与源信号的主要连接。
    QMetaObject::Connection m_sourceDestroyConnection;          ///< 源销毁连接。
    QMetaObject::Connection m_targetDestroyConnection;          ///< 目标销毁连接。
};

} // namespace BroadItem
