/// @file ReactiveBinding.cpp
/// @brief ReactiveBinding 实现 —— 元对象驱动的单向属性同步绑定。

#include <broaditem/reactive/ReactiveBinding.h>
#include <broaditem/reactive/ReactiveProperty.h>

#include <QDebug>
#include <QGraphicsObject>
#include <QMetaProperty>
#include <QPointer>

#include <initializer_list>

namespace {

/// @brief 场景位置变化跟踪器，递归监听目标对象及其父链的位置相关信号。
///
/// 负责连接目标对象以及所有 QGraphicsObject 祖先的 xChanged()/yChanged()、
/// scaleChanged()/rotationChanged()/visibleChanged()/opacityChanged() 以及
/// parentChanged() 信号。任意被跟踪对象的 parentChanged() 触发时都会重建父链连接。
/// 构造时会为目标及其所有 QGraphicsObject 祖先自动启用
/// ItemSendsGeometryChanges | ItemSendsScenePositionChanges 标志。
class ScenePosTracker : public QObject {
    Q_OBJECT
public:
    /// @brief 构造跟踪器。
    /// @param target 要跟踪的 QGraphicsObject。
    /// @param onChanged 位置可能变化时的回调。
    /// @param parent 父 QObject。
    explicit ScenePosTracker(QGraphicsObject* target,
                             std::function<void()> onChanged,
                             QObject* parent = nullptr)
        : QObject(parent)
        , m_target(target)
        , m_onChanged(std::move(onChanged))
    {
        rebuildConnections();
    }

    ~ScenePosTracker() override
    {
        clearConnections();
    }

private slots:
    /// @brief 任意祖先或目标自身的位置或视觉分量变化时调用。
    void onTrackedChanged()
    {
        if (m_onChanged) {
            m_onChanged();
        }
    }

    /// @brief 被跟踪对象的父级发生变化时重建整个父链监听并通知变化。
    void onParentChanged()
    {
        rebuildConnections();
        if (m_onChanged) {
            m_onChanged();
        }
    }

    /// @brief 任意祖先被销毁时重建父链并通知变化。
    void onAncestorDestroyed()
    {
        rebuildConnections();
        if (m_onChanged) {
            m_onChanged();
        }
    }

private:
    /// @brief 单个被跟踪对象及其信号连接。
    struct TrackedItem {
        QPointer<QGraphicsObject> object;
        QVector<QMetaObject::Connection> connections;
    };

    /// @brief 建立目标对象及所有 QGraphicsObject 祖先的位置、视觉与父级变化信号连接。
    ///
    /// 沿 parentItem() 向上遍历，对每个可转换为 QGraphicsObject 的祖先连接 xChanged()/yChanged()、
    /// scaleChanged()/rotationChanged()/visibleChanged()/opacityChanged() 以及 parentChanged()。
    /// 因 QGraphicsItem* 不是 QObject*，此处使用 dynamic_cast（qobject_cast 需要 QObject 派生指针）。
    void rebuildConnections()
    {
        clearConnections();
        if (!m_target) {
            return;
        }

        ensureGeometryChangeFlags(m_target);
        trackItem(m_target);

        QGraphicsItem* item = m_target->parentItem();
        while (item != nullptr) {
            if (auto* obj = dynamic_cast<QGraphicsObject*>(item)) {
                ensureGeometryChangeFlags(obj);
                trackItem(obj);
            }
            item = item->parentItem();
        }
    }

    /// @brief 跟踪单个对象：连接位置、视觉、父级变化信号，祖先额外监听 destroyed()。
    /// @param obj 要跟踪的 QGraphicsObject。
    void trackItem(QGraphicsObject* obj)
    {
        TrackedItem tracked;
        tracked.object = obj;
        connectSignals(obj, "onTrackedChanged()",
                       {"xChanged()", "yChanged()"}, tracked.connections);
        connectSignals(obj, "onTrackedChanged()",
                       {"scaleChanged()", "rotationChanged()",
                        "visibleChanged()", "opacityChanged()"}, tracked.connections);
        connectSignals(obj, "onParentChanged()",
                       {"parentChanged()"}, tracked.connections);

        if (obj != m_target) {
            tracked.connections.append(
                connect(obj, &QObject::destroyed, this, &ScenePosTracker::onAncestorDestroyed));
        }

        m_trackedItems.append(std::move(tracked));
    }

    /// @brief 断开所有已建立的位置、视觉与父级变化连接。
    void clearConnections()
    {
        for (const auto& tracked : m_trackedItems) {
            for (const auto& conn : tracked.connections) {
                disconnect(conn);
            }
        }
        m_trackedItems.clear();
    }

    /// @brief 若对象尚未启用几何变化标志，则自动设置。
    /// @param obj 要设置标志的 QGraphicsObject。
    static void ensureGeometryChangeFlags(QGraphicsObject* obj)
    {
        const auto requiredFlags = QGraphicsItem::ItemSendsGeometryChanges
                                   | QGraphicsItem::ItemSendsScenePositionChanges;
        if ((obj->flags() & requiredFlags) != requiredFlags) {
            obj->setFlags(obj->flags() | requiredFlags);
        }
    }

    /// @brief 将单个 QGraphicsObject 的一组信号连接到本对象的指定槽。
    ///
    /// 使用 QMetaMethod-to-QMetaMethod 连接，与 ReactiveBinding::connectSourceSignal() 的 pos 处理保持一致。
    /// @param obj 要连接的对象。
    /// @param slotName 本对象的槽名（如 "onTrackedChanged()"）。
    /// @param signalNames 要连接的信号名列表。
    /// @param outConnections 输出连接列表。
    void connectSignals(QGraphicsObject* obj, const char* slotName,
                        std::initializer_list<const char*> signalNames,
                        QVector<QMetaObject::Connection>& outConnections)
    {
        const QMetaObject* meta = obj->metaObject();
        int slotIdx = metaObject()->indexOfSlot(slotName);
        if (slotIdx < 0) {
            return;
        }
        QMetaMethod slotMethod = metaObject()->method(slotIdx);

        for (const char* sig : signalNames) {
            int idx = meta->indexOfMethod(sig);
            if (idx >= 0) {
                outConnections.append(
                    connect(obj, meta->method(idx), this, slotMethod));
            }
        }
    }

    QPointer<QGraphicsObject> m_target;                              ///< 被跟踪的目标对象。
    std::function<void()> m_onChanged;                               ///< 变化通知回调。
    QVector<TrackedItem> m_trackedItems;                             ///< 被跟踪对象及其连接。
};

} // namespace

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

/// @brief 创建只观察源属性变化的绑定，不写入目标对象。
///
/// 校验 source 非空且 sourceProperty 有效。回调函数接收源值，返回值被忽略。
/// @param source 源 QObject。
/// @param sourceProperty 源属性名。
/// @param callback 属性变化时的回调函数。
/// @param parent 父 QObject。
/// @return ReactiveBinding* 新绑定实例；参数无效时返回 nullptr。
ReactiveBinding* ReactiveBinding::createObserver(QObject* source,
                                                 const QString& sourceProperty,
                                                 Transform callback,
                                                 QObject* parent)
{
    if (!source) {
        qWarning() << "ReactiveBinding::createObserver: source is null";
        return nullptr;
    }

    if (!isValidProperty(source, sourceProperty)) {
        qWarning() << "ReactiveBinding::createObserver: invalid sourceProperty" << sourceProperty
                    << "on source" << source;
        return nullptr;
    }

    return new ReactiveBinding(source, sourceProperty, nullptr, QString(),
                               std::move(callback), parent);
}

/// @brief 创建场景位置观察者，递归监听目标对象及其所有父节点的位置变化。
///
/// 校验 source 非空且为有效的 QGraphicsObject。回调函数接收当前 scenePos() 的 QVariant(QPointF)。
/// @param source 要观察的 QGraphicsObject。
/// @param callback 场景位置变化回调。
/// @param parent 父 QObject。
/// @return ReactiveBinding* 新绑定实例；参数无效时返回 nullptr。
ReactiveBinding* ReactiveBinding::createScenePosObserver(QGraphicsObject* source,
                                                         Transform callback,
                                                         QObject* parent)
{
    if (!source) {
        qWarning() << "ReactiveBinding::createScenePosObserver: source is null";
        return nullptr;
    }

    return new ReactiveBinding(source, std::move(callback), parent);
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
    connectSourceDestroyed();

    // 连接目标对象的 destroyed() 信号，当目标销毁时禁用绑定
    if (m_target) {
        m_targetDestroyConnection = connect(m_target, &QObject::destroyed, this, [this]() {
            m_target = nullptr;
            m_enabled = false;
        });
    }
}

/// @brief 场景位置观察者专用构造。
///
/// 设置自定义源值读取器返回 source->scenePos()，并创建内部 ScenePosTracker
/// 递归监听目标对象及其父链的 xChanged()/yChanged()、scaleChanged()/rotationChanged()、
/// visibleChanged()/opacityChanged() 与 parentChanged() 信号。
/// @param source 要观察的 QGraphicsObject。
/// @param callback 场景位置变化回调。
/// @param parent 父 QObject。
ReactiveBinding::ReactiveBinding(QGraphicsObject* source,
                                 Transform callback,
                                 QObject* parent)
    : QObject(parent)
    , m_source(source)
    , m_target(nullptr)
    , m_sourceProperty()
    , m_targetProperty()
    , m_transform(std::move(callback))
    , m_enabled(true)
    , m_evaluating(false)
    , m_customSourceReader([source]() -> QVariant {
        if (!source) {
            return {};
        }
        return source->scenePos();
    })
{
    connectSourceDestroyed();

    // 跟踪器以 this 为 QObject parent，随本对象析构自动释放
    m_scenePosTracker = new ScenePosTracker(source, [this]() { evaluate(); }, this);
}

/// @brief 析构函数：内部场景位置跟踪器由 QObject 父子关系自动释放。
ReactiveBinding::~ReactiveBinding() = default;

/// @brief 静态工厂：创建响应式绑定并返回裸指针。
///
/// 执行参数校验：源和目标非空、属性名在对应对象上有效、不自绑定。
/// 校验失败时输出 qWarning 并返回 nullptr。
/// @param source 源 QObject。
/// @param sourceProperty 源属性名。
/// @param target 目标 QObject。
/// @param targetProperty 目标属性名。
/// @param transform 可选的变换函数。
/// @param parent 可选的父 QObject。
/// @return ReactiveBinding* 新绑定实例，失败时返回 nullptr。
ReactiveBinding* ReactiveBinding::create(QObject* source,
                                         const QString& sourceProperty,
                                         QObject* target,
                                         const QString& targetProperty,
                                         Transform transform,
                                         QObject* parent)
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
                               std::move(transform), parent);
}

/// @brief 销毁绑定，断开与源/目标的 QMetaObject::Connection 并禁用。
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

/// @brief 手动触发一次属性评估：读取源 → 变换 →（非观察者模式）写入目标。
///
/// 循环检测：如果 m_evaluating 已为 true（重入），则自动禁用绑定并输出警告。
/// 如果绑定已禁用或源已销毁，则跳过执行。观察者模式下目标为空，仅执行回调。
void ReactiveBinding::evaluate()
{
    if (!m_enabled || !m_source) {
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

    QVariant value;
    if (m_customSourceReader) {
        value = m_customSourceReader();
    } else {
        value = readProperty(m_source, m_sourceProperty);
    }

    // 应用变换函数
    if (m_transform) {
        value = m_transform(value);
    }

    // 仅在非观察者模式下写入目标属性
    if (m_target != nullptr && !m_targetProperty.isEmpty() && value.isValid()) {
        writeProperty(m_target, m_targetProperty, value);
    }

    m_evaluating = false;
}

/// @brief 连接源对象的 destroyed() 信号：源销毁时置空 m_source 并禁用绑定。
///
/// 普通绑定与场景位置观察者共用；普通构造时 m_scenePosTracker 尚未创建，不受影响。
void ReactiveBinding::connectSourceDestroyed()
{
    if (!m_source) {
        return;
    }
    m_sourceDestroyConnection = connect(m_source, &QObject::destroyed, this, [this]() {
        m_source = nullptr;
        m_enabled = false;
    });
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

#include "ReactiveBinding.moc"
