#include <broaditem/core/Frame.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/parser/LayoutRegistry.h>
#include <QCoreApplication>
#include <QTimer>
#include <cmath>

namespace BroadItem {

/// @brief 析构函数（out-of-line，使 unique_ptr<QObject> 在不完整类型下合法）。
Frame::~Frame() = default;

/// @brief 从 XML 文件路径构造 Frame。
/// @param xmlPath XML 布局文件路径。
/// @param ctx 属性上下文，为 nullptr 时自动创建 MapPropertyContext。
/// @return 解析并布局完成的 Frame 实例。
std::unique_ptr<Frame> Frame::fromFile(const QString& xmlPath, std::shared_ptr<PropertyContext> ctx)
{
    auto frame = std::unique_ptr<Frame>(new Frame());
    frame->m_propertyContext = ctx ? std::move(ctx) : std::make_shared<MapPropertyContext>();
    frame->setupPropertyContext();
    frame->m_rootTemplate = XmlLayoutParser::parseFile(xmlPath);
    frame->performLayout();
    return frame;
}

/// @brief 从已注册的布局 ID 构造 Frame。
/// @param layoutId 布局注册 ID。
/// @param ctx 属性上下文，为 nullptr 时自动创建 MapPropertyContext。
/// @return 加载并布局完成的 Frame 实例。
std::unique_ptr<Frame> Frame::fromRegistry(int layoutId, std::shared_ptr<PropertyContext> ctx)
{
    auto frame = std::unique_ptr<Frame>(new Frame());
    frame->m_propertyContext = ctx ? std::move(ctx) : std::make_shared<MapPropertyContext>();
    frame->setupPropertyContext();
    auto root = LayoutRegistry::instance().getLayout(layoutId);
    if (root) {
        frame->m_rootTemplate = root;
    }
    frame->performLayout();
    return frame;
}

/// @brief 连接属性上下文变更回调：绑定的属性变化时标脏并按更新策略调度重布局。
///
/// Frame 由工厂以 unique_ptr 堆分配返回，[this] 捕获在 Frame 生命周期内安全。
void Frame::setupPropertyContext()
{
    if (m_propertyContext) {
        m_propertyContext->setOnChanged([this](const QString& name, const QVariant&) {
            if (m_rootTemplate && m_rootTemplate->bindsProperty(name))
                requestUpdate();
        });
    }
}

/// @brief 标脏并按更新策略调度重布局。
///
/// Synchronous（或无 QCoreApplication 的无事件循环环境）立即执行重布局动作；
/// Coalesced 经 QTimer::singleShot(0) 排入事件循环，m_updateScheduled 守卫位
/// 保证同一回合内多次变更只排入一次任务。singleShot 以 m_timerContext 为
/// context，Frame 销毁时待定任务被 Qt 自动丢弃，不会悬挂回调。
void Frame::requestUpdate()
{
    m_dirty = true;

    if (m_updatePolicy == UpdatePolicy::Synchronous || !QCoreApplication::instance()) {
        m_relayoutAction();
        return;
    }

    if (m_updateScheduled)
        return;
    m_updateScheduled = true;

    if (!m_timerContext)
        m_timerContext = std::make_unique<QObject>();

    QTimer::singleShot(0, m_timerContext.get(), [this] {
        if (!m_updateScheduled)
            return;  // 已被 flush() 抢先执行
        m_updateScheduled = false;
        m_relayoutAction();
    });
}

/// @brief 立即执行待定的合并更新；无待定时无操作（幂等）。
void Frame::flush()
{
    if (!m_updateScheduled)
        return;
    m_updateScheduled = false;
    m_relayoutAction();
}

/// @brief 替换属性上下文并重新连接变更通知；实例树标脏以待下次重新物化。
/// @param ctx 新的属性上下文。
void Frame::setPropertyContext(std::shared_ptr<PropertyContext> ctx)
{
    m_propertyContext = std::move(ctx);
    m_dirty = true;
    setupPropertyContext();
}

/// @brief 设置动态属性值，如已绑定则通过 onChanged 回调触发重新布局。
/// @param name 属性名。
/// @param value 属性值。
void Frame::setDynamicProperty(const QString& name, const QVariant& value)
{
    if (m_propertyContext)
        m_propertyContext->setProperty(name, value);
}

/// @brief 获取动态属性值。
/// @param name 属性名。
/// @return 属性值，不存在时返回无效 QVariant。
QVariant Frame::dynamicProperty(const QString& name) const
{
    return m_propertyContext ? m_propertyContext->property(name) : QVariant();
}

/// @brief 检查动态属性是否存在。
/// @param name 属性名。
/// @return 存在返回 true，否则 false。
bool Frame::hasDynamicProperty(const QString& name) const
{
    return m_propertyContext && m_propertyContext->hasProperty(name);
}

/// @brief 运行完整的物化（标脏时）→测量→布局管线，返回计算后的尺寸。
///
/// 数据经 setProperty 变更必然触发 onChanged 标脏，因此仅当 m_dirty
/// 或节点树尚不存在时才重新物化，语义与旧版逐次展开等价。
/// 根模板为控制元素时取物化结果的第一个节点（保持旧版"只取第一项"行为）。
///
/// @param availableWidth  可用宽度，-1 表示无限制。
/// @param availableHeight 可用高度，-1 表示无限制。
/// @return 布局后的帧尺寸。
QSizeF Frame::performLayout(double availableWidth, double availableHeight)
{
    if (!m_rootTemplate)
        return QSizeF();

    // 运行时诊断去重作用域：覆盖物化/测量/布局全管线
    const Diagnostics::RuntimeScope runtimeScope(m_rootTemplate.get());

    m_context.ctx = m_propertyContext.get();

    if (m_dirty || !m_rootNode) {
        auto nodes = m_rootTemplate->materializeChildren(m_context);
        m_rootNode = nodes.empty() ? nullptr : std::move(nodes[0]);
        m_dirty = false;
    }

    if (!m_rootNode) {
        m_boundingRect = QRectF();
        return QSizeF();
    }

    LayoutConstraints constraints;
    constraints.availableWidth = availableWidth;
    constraints.availableHeight = availableHeight;

    auto result = m_rootNode->element->measure(m_context, constraints, *m_rootNode);
    m_boundingRect = QRectF(0, 0, result.intrinsicSize.width(), result.intrinsicSize.height());

    QRectF rootRect(0, 0, result.intrinsicSize.width(), result.intrinsicSize.height());
    m_rootNode->element->layout(m_context, rootRect, *m_rootNode);

    return m_boundingRect.size();
}

/// @brief 使用给定的 painter 将实例节点树绘制到目标设备。
///
/// 同步 m_context.ctx（因该成员为 mutable）后调用根节点模板的 render()。
/// 调用者有责任确保 QPainter 已正确初始化且处于活动状态。
///
/// @param painter 目标 QPainter，不可为 nullptr。
void Frame::paint(QPainter* painter) const
{
    if (m_rootNode) {
        const Diagnostics::RuntimeScope runtimeScope(m_rootTemplate.get());
        m_context.ctx = m_propertyContext.get();
        m_rootNode->element->render(painter, m_context, *m_rootNode);
    }
}

/// @brief 返回帧的尺寸（最近一次 performLayout 计算的结果）。
/// @return 帧尺寸。首次 performLayout 前返回 (0,0)。
QSizeF Frame::size() const
{
    return m_boundingRect.size();
}

/// @brief 将帧渲染为含设备像素比元数据的高质量 QImage。
///
/// 按 dpr 倍率创建 QImage，用透明色填充后通过缩放后的 QPainter 绘制。
/// 元素以逻辑坐标绘制，缩放变换确保在 dpr > 1 时保持清晰。
///
/// @param dpr 设备像素比，默认 1.0。
/// @return 渲染后的图像。实例树为空或 boundingRect 为空时返回空的 QImage。
QImage Frame::toImage(double dpr) const
{
    if (!m_rootNode || m_boundingRect.isEmpty())
        return QImage();

    QSizeF scaled = m_boundingRect.size() * dpr;
    QImage image(static_cast<int>(std::ceil(scaled.width())),
                 static_cast<int>(std::ceil(scaled.height())),
                 QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    // 注意：不要额外 painter.scale(dpr, dpr)——QPainter 在带 devicePixelRatio 的
    // QImage 上 begin 时已自动应用该比例，再缩放会重复放大内容。
    paint(&painter);
    painter.end();

    return image;
}

/// @brief 获取当前属性上下文。
/// @return 属性上下文指针，可能为 nullptr。
std::shared_ptr<PropertyContext> Frame::propertyContext() const
{
    return m_propertyContext;
}

/// @brief 检查指定属性是否被模板元素树中的元素绑定。
/// @param name 属性名。
/// @return 绑定返回 true，否则 false。
bool Frame::bindsProperty(const QString& name) const
{
    return m_rootTemplate && m_rootTemplate->bindsProperty(name);
}

} // namespace BroadItem
