#include <broaditem/reactive/ReactiveBinding.h>
#include <broaditem/reactive/ReactiveProperty.h>

#include <QDebug>
#include <QMetaProperty>

namespace BroadItem {

/// @brief 验证属性名在指定对象上有效。
///
/// 属性必须存在于对象的 metaObject 中、可写、且有 NOTIFY 信号。
/// 特殊处理：pos 属性无 NOTIFY 信号但通过 xChanged()/yChanged() 连接，单独允许。
/// @param obj 要检查的 QObject。
/// @param prop 属性名。
/// @return true 表示属性有效。
bool ReactiveBinding::isValidProperty(const QObject* obj, const QString& prop)
{
    if (!obj || prop.isEmpty()) {
        return false;
    }

    const QMetaObject* meta = obj->metaObject();
    int propIdx = meta->indexOfProperty(prop.toLatin1().constData());
    if (propIdx < 0) {
        return false;
    }

    QMetaProperty metaProp = meta->property(propIdx);
    if (!metaProp.isWritable()) {
        return false;
    }

    // pos 属性没有 NOTIFY 信号，通过 xChanged()/yChanged() 特判支持
    if (prop == Property::Pos) {
        return true;
    }

    // 其他属性必须有 NOTIFY 信号
    return metaProp.hasNotifySignal();
}

/// @brief 私有构造，通过 create() 工厂创建。
///
/// 存储参数，连接源信号的 NOTIFY 信号或分量信号，连接目标/源的 destroyed() 信号。
/// @param source 源对象。
/// @param sourceProperty 源属性名。
/// @param target 目标对象。
/// @param targetProperty 目标属性名。
/// @param transform 可选的变换函数。
/// @param parent 父 QObject。
ReactiveBinding::ReactiveBinding(QObject* source,
                                 QString sourceProperty,
                                 QObject* target,
                                 QString targetProperty,
                                 Transform transform,
                                 QObject* parent)
    : QObject(parent)
    , m_source(source)
    , m_target(target)
    , m_sourceProperty(std::move(sourceProperty))
    , m_targetProperty(std::move(targetProperty))
    , m_transform(std::move(transform))
    , m_enabled(true)
    , m_evaluating(false)
{
    connectSourceSignal();

    // 连接源对象的 destroyed() 信号，当源销毁时禁用绑定
    if (m_source) {
        m_sourceDestroyConnection = connect(m_source, &QObject::destroyed, this, [this]() {
            m_source = nullptr;
            m_enabled = false;
        });
    }

    // 连接目标对象的 destroyed() 信号，当目标销毁时禁用绑定
    if (m_target) {
        m_targetDestroyConnection = connect(m_target, &QObject::destroyed, this, [this]() {
            m_target = nullptr;
            m_enabled = false;
        });
    }
}

/// @brief 静态工厂：创建响应式绑定并返回裸指针。
///
/// 执行参数校验：源和目标非空、属性名在对应对象上有效、不自绑定。
/// 校验失败时输出 qWarning 并返回 nullptr。
/// @param source 源 QObject。
/// @param sourceProperty 源属性名。
/// @param target 目标 QObject。
/// @param targetProperty 目标属性名。
/// @param transform 可选的变换函数。
/// @return ReactiveBinding* 新绑定实例，失败时返回 nullptr。
ReactiveBinding* ReactiveBinding::create(QObject* source,
                                         const QString& sourceProperty,
                                         QObject* target,
                                         const QString& targetProperty,
                                         Transform transform)
{
    if (!source) {
        qWarning() << "ReactiveBinding::create: source is null";
        return nullptr;
    }

    if (!target) {
        qWarning() << "ReactiveBinding::create: target is null";
        return nullptr;
    }

    if (!isValidProperty(source, sourceProperty)) {
        qWarning() << "ReactiveBinding::create: invalid sourceProperty" << sourceProperty
                    << "on source" << source;
        return nullptr;
    }

    if (!isValidProperty(target, targetProperty)) {
        qWarning() << "ReactiveBinding::create: invalid targetProperty" << targetProperty
                    << "on target" << target;
        return nullptr;
    }

    if (source == target && sourceProperty == targetProperty) {
        qWarning() << "ReactiveBinding::create: self-binding detected, source and target are the same object with property"
                    << sourceProperty;
        return nullptr;
    }

    return new ReactiveBinding(source, sourceProperty, target, targetProperty,
                               std::move(transform));
}

/// @brief 销毁绑定，断开所有 QMetaObject::Connection 并禁用。
void ReactiveBinding::destroy()
{
    m_enabled = false;

    for (const auto& conn : m_signalConnections) {
        disconnect(conn);
    }
    m_signalConnections.clear();

    disconnect(m_sourceDestroyConnection);
    disconnect(m_targetDestroyConnection);

    m_source = nullptr;
    m_target = nullptr;
}

/// @brief 启用或禁用绑定。
/// @param enabled true 启用，false 禁用。
void ReactiveBinding::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

/// @brief 查询绑定是否启用。
/// @return true 表示绑定已启用。
bool ReactiveBinding::isEnabled() const
{
    return m_enabled;
}

/// @brief 获取源对象。
/// @return 源 QObject 指针。
QObject* ReactiveBinding::source() const
{
    return m_source;
}

/// @brief 获取目标对象。
/// @return 目标 QObject 指针。
QObject* ReactiveBinding::target() const
{
    return m_target;
}

/// @brief 获取源属性名。
/// @return 源属性名字符串。
QString ReactiveBinding::sourceProperty() const
{
    return m_sourceProperty;
}

/// @brief 获取目标属性名。
/// @return 目标属性名字符串。
QString ReactiveBinding::targetProperty() const
{
    return m_targetProperty;
}

/// @brief 源属性变化槽，触发属性评估。
void ReactiveBinding::onSourceChanged()
{
    evaluate();
}

/// @brief 手动触发一次属性评估：读取源 → 变换 → 写入目标。
///
/// 循环检测：如果 m_evaluating 已为 true（重入），则自动禁用绑定并输出警告。
/// 如果绑定已禁用、源或目标已销毁，则跳过执行。
void ReactiveBinding::evaluate()
{
    if (!m_enabled || !m_source || !m_target) {
        return;
    }

    // 循环检测：如果已经在评估中，说明存在循环依赖
    if (m_evaluating) {
        qWarning() << "ReactiveBinding::evaluate: cycle detected, disabling binding"
                    << "source:" << m_source << "property:" << m_sourceProperty
                    << "target:" << m_target << "property:" << m_targetProperty;
        m_enabled = false;
        return;
    }

    m_evaluating = true;

    QVariant value = readProperty(m_source, m_sourceProperty);

    // 应用变换函数
    if (m_transform) {
        value = m_transform(value);
    }

    // 写入目标属性
    if (value.isValid()) {
        writeProperty(m_target, m_targetProperty, value);
    }

    m_evaluating = false;
}

/// @brief 连接源对象的对应属性变化信号。
///
/// 对 scale/rotation/opacity/visible：通过 QMetaProperty::notifySignal() 获取
/// 信号的 QMetaMethod，用 QMetaMethod-to-QMetaMethod connect 连接到 onSourceChanged()。
/// 对 pos：连接 xChanged() 和 yChanged() 两个信号（pos 无 NOTIFY 信号）。
void ReactiveBinding::connectSourceSignal()
{
    if (!m_source) {
        return;
    }

    // 获取 onSourceChanged 槽的 QMetaMethod
    int slotIdx = metaObject()->indexOfSlot("onSourceChanged()");
    if (slotIdx < 0) {
        qWarning() << "ReactiveBinding::connectSourceSignal: onSourceChanged slot not found";
        return;
    }
    QMetaMethod slotMethod = metaObject()->method(slotIdx);

    // pos 属性没有 NOTIFY 信号，连接 xChanged() 和 yChanged()
    if (m_sourceProperty == Property::Pos) {
        const QMetaObject* meta = m_source->metaObject();
        int xIdx = meta->indexOfMethod("xChanged()");
        int yIdx = meta->indexOfMethod("yChanged()");
        if (xIdx >= 0) {
            m_signalConnections.append(
                connect(m_source, meta->method(xIdx), this, slotMethod));
        }
        if (yIdx >= 0) {
            m_signalConnections.append(
                connect(m_source, meta->method(yIdx), this, slotMethod));
        }
        return;
    }

    // 其他属性：通过 QMetaProperty::notifySignal() 获取 NOTIFY 信号
    const QMetaObject* meta = m_source->metaObject();
    int propIdx = meta->indexOfProperty(m_sourceProperty.toLatin1().constData());
    if (propIdx < 0) {
        qWarning() << "ReactiveBinding::connectSourceSignal: property not found:"
                    << m_sourceProperty;
        return;
    }

    QMetaProperty metaProp = meta->property(propIdx);
    if (!metaProp.hasNotifySignal()) {
        qWarning() << "ReactiveBinding::connectSourceSignal: property has no notify signal:"
                    << m_sourceProperty;
        return;
    }

    QMetaMethod notifySignal = metaProp.notifySignal();
    m_signalConnections.append(
        connect(m_source, notifySignal, this, slotMethod));
}

/// @brief 从 QObject 读取属性值。
///
/// 通过 QObject::property() 统一读取，对 QGraphicsObject 的 pos 属性同样有效。
/// @param obj QObject 指针。
/// @param prop 属性名。
/// @return 属性值 QVariant。
QVariant ReactiveBinding::readProperty(const QObject* obj, const QString& prop)
{
    if (!obj) {
        return {};
    }

    return obj->property(prop.toLatin1().constData());
}

/// @brief 向 QObject 写入属性值。
///
/// 通过 QObject::setProperty() 统一写入。若写入失败则输出 warning。
/// @param obj QObject 指针。
/// @param prop 属性名。
/// @param value 要写入的值。
void ReactiveBinding::writeProperty(QObject* obj, const QString& prop,
                                    const QVariant& value)
{
    if (!obj) {
        return;
    }

    bool ok = obj->setProperty(prop.toLatin1().constData(), value);
    if (!ok) {
        qWarning() << "ReactiveBinding::writeProperty: failed to set property"
                    << prop << "on" << obj << "value type:" << value.typeName();
    }
}

} // namespace BroadItem
