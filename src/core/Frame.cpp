#include <broaditem/core/Frame.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/parser/LayoutRegistry.h>
#include <cmath>

namespace BroadItem {

/// @brief 从 XML 文件路径构造 Frame。
/// @param xmlPath XML 布局文件路径。
/// @param ctx 属性上下文，为 nullptr 时自动创建 MapPropertyContext。
/// @return 解析并布局完成的 Frame 实例。
Frame Frame::fromFile(const QString& xmlPath, std::shared_ptr<PropertyContext> ctx)
{
    Frame frame;
    frame.m_propertyContext = ctx ? std::move(ctx) : std::make_shared<MapPropertyContext>();
    frame.setupPropertyContext();
    frame.m_rootElement = XmlLayoutParser::parseFile(xmlPath);
    frame.performLayout();
    return frame;
}

/// @brief 从已注册的布局 ID 构造 Frame。
/// @param layoutId 布局注册 ID。
/// @param ctx 属性上下文，为 nullptr 时自动创建 MapPropertyContext。
/// @return 加载并布局完成的 Frame 实例。
Frame Frame::fromRegistry(int layoutId, std::shared_ptr<PropertyContext> ctx)
{
    Frame frame;
    frame.m_propertyContext = ctx ? std::move(ctx) : std::make_shared<MapPropertyContext>();
    frame.setupPropertyContext();
    auto root = LayoutRegistry::instance().getLayout(layoutId);
    if (root) {
        frame.m_rootElement = root;
    }
    frame.performLayout();
    return frame;
}

/// @brief 连接属性上下文变更回调，在绑定的属性变化时触发重新布局。
void Frame::setupPropertyContext()
{
    if (m_propertyContext) {
        /// @note 通过 [this] 捕获确保回调可访问 m_rootElement 和 performLayout。
        ///       调用者应通过 RVO 接收 fromFile/fromRegistry 的返回值，避免移动
        ///       导致 this 悬空指针。
        m_propertyContext->setOnChanged([this](const QString& name, const QVariant&) {
            if (m_rootElement && m_rootElement->bindsProperty(name)) {
                performLayout();
            }
        });
    }
}

/// @brief 替换属性上下文并重新连接变更通知。
/// @param ctx 新的属性上下文。
void Frame::setPropertyContext(std::shared_ptr<PropertyContext> ctx)
{
    m_propertyContext = std::move(ctx);
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

/// @brief 运行完整的测量→布局管线，返回计算后的尺寸。
///
/// 设置上下文、运行根元素的 measure()，将 intrinsicSize 存入 m_boundingRect，
/// 然后运行 layout() 分配最终几何。
///
/// @param availableWidth  可用宽度，-1 表示无限制。
/// @param availableHeight 可用高度，-1 表示无限制。
/// @return 布局后的帧尺寸。
QSizeF Frame::performLayout(double availableWidth, double availableHeight)
{
    if (!m_rootElement)
        return QSizeF();

    m_context.ctx = m_propertyContext.get();

    LayoutConstraints constraints;
    constraints.availableWidth = availableWidth;
    constraints.availableHeight = availableHeight;

    auto result = m_rootElement->measure(m_context, constraints);
    m_boundingRect = QRectF(0, 0, result.intrinsicSize.width(), result.intrinsicSize.height());

    QRectF rootRect(0, 0, result.intrinsicSize.width(), result.intrinsicSize.height());
    m_rootElement->layout(m_context, rootRect);

    return m_boundingRect.size();
}

/// @brief 使用给定的 painter 将元素树绘制到目标设备。
///
/// 同步 m_context.ctx（因该成员为 mutable）后调用根元素的 render()。
/// 调用者有责任确保 QPainter 已正确初始化且处于活动状态。
///
/// @param painter 目标 QPainter，不可为 nullptr。
void Frame::paint(QPainter* painter) const
{
    if (m_rootElement) {
        m_context.ctx = m_propertyContext.get();
        m_rootElement->render(painter, m_context);
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
/// @return 渲染后的图像。m_rootElement 为 nullptr 或 boundingRect 为空时返回空的 QImage。
QImage Frame::toImage(double dpr) const
{
    if (!m_rootElement || m_boundingRect.isEmpty())
        return QImage();

    QSizeF scaled = m_boundingRect.size() * dpr;
    QImage image(static_cast<int>(std::ceil(scaled.width())),
                 static_cast<int>(std::ceil(scaled.height())),
                 QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(dpr, dpr);
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

/// @brief 检查指定属性是否被元素树中的元素绑定。
/// @param name 属性名。
/// @return 绑定返回 true，否则 false。
bool Frame::bindsProperty(const QString& name) const
{
    return m_rootElement && m_rootElement->bindsProperty(name);
}

} // namespace BroadItem
