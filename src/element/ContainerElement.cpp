#include <broaditem/element/ContainerElement.h>
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

/// @brief 默认构造函数。
ContainerElement::ContainerElement() = default;

/// @brief 从 XML 元素解析盒模型属性。
/// @param xml The DOM element to parse.
void ContainerElement::parse(const QDomElement& xml)
{
    parseBoxModel(xml);
}

/// @brief 测量容器：无内容时返回盒模型尺寸；否则测量内容并添加装饰。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @return The measured size of the container.
MeasureResult ContainerElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    if (!m_content) {
        QSizeF sz(boxModelWidth(), boxModelHeight());
        return MeasureResult{sz};
    }

    LayoutConstraints childConstraints = constraints;
    double decoW = boxModelWidth();
    double decoH = boxModelHeight();

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    auto result = m_content->measure(ctx, childConstraints);
    QSizeF sz(result.intrinsicSize.width() + decoW, result.intrinsicSize.height() + decoH);
    return MeasureResult{sz};
}

/// @brief 存储分配的矩形并在内容区域内布局内容。
/// @param ctx The layout context.
/// @param rect The bounding rectangle assigned to this container.
void ContainerElement::layout(const LayoutContext& ctx, const QRectF& rect)
{
    m_rect = rect;
    if (!m_content)
        return;

    QRectF cr = contentRect(rect);
    m_content->layout(ctx, cr);
}

/// @brief 渲染盒模型装饰，然后委托给内容元素。
/// @param painter The QPainter to render onto.
/// @param ctx The layout context.
void ContainerElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    renderBoxModel(painter, m_rect);
    if (m_content)
        m_content->render(painter, ctx);
}

/// @brief 检查内容元素是否绑定指定属性。
/// @param name The property name to check.
/// @return True if content exists and binds the property.
bool ContainerElement::bindsProperty(const QString& name) const
{
    return m_content && m_content->bindsProperty(name);
}

/// @brief 设置此容器的单一内容子元素。
/// @param content The element to wrap.
void ContainerElement::setContent(ElementPtr content)
{
    m_content = std::move(content);
}

/// @brief 将绑定解析传播到内容子元素。
/// @param ctx The layout context with property values.
void ContainerElement::resolveBindings(const LayoutContext& ctx)
{
    if (m_content)
        m_content->resolveBindings(ctx);
}

/// @brief 创建此容器的深拷贝，包括其内容。
/// @return A new ContainerElement with cloned properties and content.
ElementPtr ContainerElement::clone() const
{
    auto copy = std::make_shared<ContainerElement>();
    copy->m_margin = m_margin;
    copy->m_border = m_border;
    copy->m_background = m_background;
    copy->m_padding = m_padding;
    copy->m_rect = m_rect;
    if (m_content)
        copy->m_content = m_content->clone();
    return copy;
}

} // namespace BroadItem
