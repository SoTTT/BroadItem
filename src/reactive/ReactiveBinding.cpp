#include <broaditem/reactive/ReactiveBinding.h>
#include <broaditem/reactive/ObservableGraphicsObject.h>
#include <broaditem/reactive/ReactiveProperty.h>

#include <QDebug>
#include <QGraphicsObject>

namespace BroadItem {

/// @brief 检查属性名是否属于支持的属性。
/// @param prop 属性名。
/// @return true 表示属性名有效。
bool ReactiveBinding::isValidProperty(const QString& prop)
{
    return prop == Property::Pos
        || prop == Property::Scale
        || prop == Property::Rotation
        || prop == Property::Opacity
        || prop == Property::Visible;
}

/// @brief 私有构造，通过 create() 工厂创建。
///
/// 存储参数，连接源信号和目标/源的 destroyed() 信号，默认启用绑定。
/// @param source 源图形对象。
/// @param sourceProperty 源属性名。
/// @param target 目标图形对象。
/// @param targetProperty 目标属性名。
/// @param transform 可选的变换函数。
/// @param parent 父 QObject。
ReactiveBinding::ReactiveBinding(ObservableGraphicsObject* source,
                                 QString sourceProperty,
                                 ObservableGraphicsObject* target,
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
/// 执行参数校验：源和目标非空、属性名有效、不自绑定。
/// 校验失败时输出 qWarning 并返回 nullptr。
/// @param source 源 ObservableGraphicsObject。
/// @param sourceProperty 源属性名。
/// @param target 目标 ObservableGraphicsObject。
/// @param targetProperty 目标属性名。
/// @param transform 可选的变换函数。
/// @return ReactiveBinding* 新绑定实例，失败时返回 nullptr。
ReactiveBinding* ReactiveBinding::create(ObservableGraphicsObject* source,
                                         const QString& sourceProperty,
                                         ObservableGraphicsObject* target,
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

    if (!isValidProperty(sourceProperty)) {
        qWarning() << "ReactiveBinding::create: invalid sourceProperty" << sourceProperty;
        return nullptr;
    }

    if (!isValidProperty(targetProperty)) {
        qWarning() << "ReactiveBinding::create: invalid targetProperty" << targetProperty;
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

    disconnect(m_signalConnection);
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
/// @return 源 ObservableGraphicsObject 指针。
ObservableGraphicsObject* ReactiveBinding::source() const
{
    return m_source;
}

/// @brief 获取目标对象。
/// @return 目标 ObservableGraphicsObject 指针。
ObservableGraphicsObject* ReactiveBinding::target() const
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

/// @brief 手动触发一次属性评估：读取源 → 变换 → 写入目标。
///
/// 循环检测：如果 m_evaluating 已为 true（重入），则自动禁用绑定并输出警告。
/// 如果绑定已禁用、源或目标已销毁，则跳过执行。
/// 类型检查：读取值与目标属性类型不匹配时输出警告并跳过写入。
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

    // 类型检查：写入前验证值类型与目标属性匹配
    if (value.isValid()) {
        writeProperty(m_target, m_targetProperty, value);
    }

    m_evaluating = false;
}

/// @brief 连接源对象的对应属性变化信号。
///
/// 根据 m_sourceProperty 连接到 ObservableGraphicsObject 的对应信号。
/// 信号触发时调用 evaluate() 执行属性同步。
void ReactiveBinding::connectSourceSignal()
{
    if (!m_source) {
        return;
    }

    if (m_sourceProperty == Property::Pos) {
        m_signalConnection = connect(m_source, &ObservableGraphicsObject::positionChanged,
                                     this, [this](const QPointF&) { evaluate(); });
    } else if (m_sourceProperty == Property::Scale) {
        m_signalConnection = connect(m_source, &ObservableGraphicsObject::scaleChanged,
                                     this, [this](qreal) { evaluate(); });
    } else if (m_sourceProperty == Property::Rotation) {
        m_signalConnection = connect(m_source, &ObservableGraphicsObject::rotationChanged,
                                     this, [this](qreal) { evaluate(); });
    } else if (m_sourceProperty == Property::Opacity) {
        m_signalConnection = connect(m_source, &ObservableGraphicsObject::opacityChanged,
                                     this, [this](qreal) { evaluate(); });
    } else if (m_sourceProperty == Property::Visible) {
        m_signalConnection = connect(m_source, &ObservableGraphicsObject::visibilityChanged,
                                     this, [this](bool) { evaluate(); });
    }
}

/// @brief 从图形对象读取属性值。
/// @param obj 图形对象指针。
/// @param prop 属性名。
/// @return 属性值 QVariant。
QVariant ReactiveBinding::readProperty(const ObservableGraphicsObject* obj, const QString& prop)
{
    if (!obj) {
        return {};
    }

    if (prop == Property::Pos) {
        return obj->pos();
    } else if (prop == Property::Scale) {
        return obj->scale();
    } else if (prop == Property::Rotation) {
        return obj->rotation();
    } else if (prop == Property::Opacity) {
        return obj->opacity();
    } else if (prop == Property::Visible) {
        return obj->isVisible();
    }

    qWarning() << "ReactiveBinding::readProperty: unknown property" << prop;
    return {};
}

/// @brief 向图形对象写入属性值。
/// @param obj 图形对象指针。
/// @param prop 属性名。
/// @param value 要写入的值。
void ReactiveBinding::writeProperty(ObservableGraphicsObject* obj, const QString& prop,
                                    const QVariant& value)
{
    if (!obj) {
        return;
    }

    if (prop == Property::Pos) {
        if (value.canConvert<QPointF>()) {
            obj->setPos(value.toPointF());
        } else {
            qWarning() << "ReactiveBinding::writeProperty: type mismatch for pos, expected QPointF, got"
                        << value.typeName();
        }
    } else if (prop == Property::Scale) {
        if (value.canConvert<qreal>()) {
            obj->setScale(value.value<qreal>());
        } else {
            qWarning() << "ReactiveBinding::writeProperty: type mismatch for scale, expected qreal, got"
                        << value.typeName();
        }
    } else if (prop == Property::Rotation) {
        if (value.canConvert<qreal>()) {
            obj->setRotation(value.value<qreal>());
        } else {
            qWarning() << "ReactiveBinding::writeProperty: type mismatch for rotation, expected qreal, got"
                        << value.typeName();
        }
    } else if (prop == Property::Opacity) {
        if (value.canConvert<qreal>()) {
            obj->setOpacity(value.value<qreal>());
        } else {
            qWarning() << "ReactiveBinding::writeProperty: type mismatch for opacity, expected qreal, got"
                        << value.typeName();
        }
    } else if (prop == Property::Visible) {
        if (value.canConvert<bool>()) {
            obj->setVisible(value.toBool());
        } else {
            qWarning() << "ReactiveBinding::writeProperty: type mismatch for visible, expected bool, got"
                        << value.typeName();
        }
    } else {
        qWarning() << "ReactiveBinding::writeProperty: unknown property" << prop;
    }
}

} // namespace BroadItem
