#pragma once

#include <QObject>
#include <QMetaObject>
#include <QVariant>
#include <functional>

namespace BroadItem {

class ObservableGraphicsObject;

/// @brief 响应式绑定类，将源对象的属性变化单向同步到目标对象。
///
/// ReactiveBinding 监听 ObservableGraphicsObject 的属性变化信号，
/// 当源属性发生变化时读取源值、应用可选的变换函数，然后写入目标对象的对应属性。
/// 支持 pos、scale、rotation、opacity、visible 五种属性。
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
    /// 如果属性名无效或源/目标指针为空，返回 nullptr 并输出 qWarning。
    /// 如果源和目标相同且属性相同（自绑定），输出 qWarning 并返回 nullptr。
    /// @param source 源 ObservableGraphicsObject 指针。
    /// @param sourceProperty 源属性名（"pos"、"scale"、"rotation"、"opacity"、"visible"）。
    /// @param target 目标 ObservableGraphicsObject 指针。
    /// @param targetProperty 目标属性名。
    /// @param transform 可选的变换函数，默认为 nullptr（直通）。
    /// @return ReactiveBinding* 新绑定实例，失败时返回 nullptr。
    static ReactiveBinding* create(ObservableGraphicsObject* source,
                                   const QString& sourceProperty,
                                   ObservableGraphicsObject* target,
                                   const QString& targetProperty,
                                   Transform transform = nullptr);

    /// @brief 销毁绑定，断开所有连接并标记为无效。
    void destroy();

    /// @brief 启用或禁用绑定。
    /// @param enabled true 启用，false 禁用。
    void setEnabled(bool enabled);

    /// @brief 查询绑定是否启用。
    /// @return true 表示绑定已启用。
    [[nodiscard]] bool isEnabled() const;

    /// @brief 手动触发一次属性评估：读取源属性，变换，写入目标。
    ///
    /// 如果绑定已禁用或 m_evaluating 标志为 true（重入），则跳过执行。
    void evaluate();

    /// @brief 获取源对象。
    /// @return 源 ObservableGraphicsObject 指针，可能为 nullptr（已销毁）。
    [[nodiscard]] ObservableGraphicsObject* source() const;

    /// @brief 获取目标对象。
    /// @return 目标 ObservableGraphicsObject 指针，可能为 nullptr（已销毁）。
    [[nodiscard]] ObservableGraphicsObject* target() const;

    /// @brief 获取源属性名。
    /// @return 源属性名字符串。
    [[nodiscard]] QString sourceProperty() const;

    /// @brief 获取目标属性名。
    /// @return 目标属性名字符串。
    [[nodiscard]] QString targetProperty() const;

private:
    /// @brief 私有构造，通过 create() 工厂创建。
    ReactiveBinding(ObservableGraphicsObject* source,
                    QString sourceProperty,
                    ObservableGraphicsObject* target,
                    QString targetProperty,
                    Transform transform,
                    QObject* parent = nullptr);

    /// @brief 连接源对象的对应属性变化信号。
    void connectSourceSignal();

    /// @brief 从图形对象读取属性值。
    static QVariant readProperty(const ObservableGraphicsObject* obj, const QString& prop);

    /// @brief 向图形对象写入属性值。
    static void writeProperty(ObservableGraphicsObject* obj, const QString& prop, const QVariant& value);

    /// @brief 验证属性名是否有效。
    static bool isValidProperty(const QString& prop);

    ObservableGraphicsObject* m_source;     ///< 源图形对象。
    ObservableGraphicsObject* m_target;     ///< 目标图形对象。
    QString m_sourceProperty;               ///< 源属性名。
    QString m_targetProperty;               ///< 目标属性名。
    Transform m_transform;                  ///< 可选的变换函数。
    bool m_enabled;                         ///< 绑定是否启用。
    bool m_evaluating;                      ///< 循环检测标志，防止 evaluate() 重入。

    QMetaObject::Connection m_signalConnection;       ///< 与源信号的主要连接。
    QMetaObject::Connection m_sourceDestroyConnection;  ///< 源销毁连接。
    QMetaObject::Connection m_targetDestroyConnection;  ///< 目标销毁连接。
};

} // namespace BroadItem
