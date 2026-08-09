#include <broaditem/element/image/ImageElement.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/compat/QtCompat.h>
#include "ImageCache.h"
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 ImageElement 支持的 XML 属性集合。
/// @return 静态集合引用：src、keep-aspect、width/height 与全部盒模型属性。
const QSet<QString>& ImageElement::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{
        "src", "keep-aspect", "width", "height"
    } + boxModelAttributeNames();
    return attrs;
}

/// @brief 返回实际参与解析/求值的属性集合（SizedElement 集合 + src/keep-aspect）。
const QSet<QString>& ImageElement::resolvedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{
        "src", "keep-aspect"
    } + SizedElement::resolvedAttributes();
    return attrs;
}

/// @brief 解析图像属性：src 字面量与 keep-aspect。
///
/// src 与 b:src 的互斥由 parseBindings（SizedElement::parse 链）既有逻辑裁决，
/// 此处只读取字面量；字面量读取一律走 hasLiteralAttribute/literalAttribute
/// （QDom 命名空间陷阱：hasAttributeNS(QString(), ...) 匹配不到无命名空间字面量）。
///
/// @param xml 要解析的 DOM 元素。
void ImageElement::parse(const QDomElement& xml)
{
    SizedElement::parse(xml);
    parseBoxModel(xml);
    validateAttributes(xml);

    m_srcLiteral = literalAttribute(xml, "src");
    if (hasLiteralAttribute(xml, "keep-aspect"))
        m_keepAspect = parseBool(literalAttribute(xml, "keep-aspect"));
}

/// @brief 物化：求值样式与尺寸，解析 src 并经进程级缓存加载图像。
///
/// 缓存语义（ImageCache：路径键、进程级、互斥锁保护，模板零 mutable 状态）：
/// - Hit：直接复用，避免重复磁盘加载；
/// - Failed：跳过加载，静默空白（告警去重由运行时诊断统一机制承担）；
/// - Miss：尝试 QPixmap(path) 加载，成功写成功表，失败报 BI-R-010 一次并写失败表；
///   缓存永不淘汰，生命周期随进程。
///
/// src 为空串（未指定或绑定求值为空）时静默空白——node->pixmap 保持为空，
/// 渲染阶段只画盒模型装饰，不绘制占位错误图。
///
/// @param ctx 布局上下文，用于解析数据绑定。
/// @return 新创建的 ImageNode 实例节点。
std::unique_ptr<Node> ImageElement::materialize(const LayoutContext& ctx) const
{
    auto node = makeUnique<ImageNode>();
    node->element = this;
    resolveStyle(ctx, node->style);
    resolveSize(ctx, node->style);

    node->srcPath = resolveString("src", ctx, m_srcLiteral);
    node->keepAspect = resolveBool("keep-aspect", ctx, m_keepAspect);

    const QString& path = node->srcPath;
    if (!path.isEmpty()) {
        QPixmap pixmap;
        switch (ImageCache::lookup(path, pixmap)) {
        case ImageCache::Lookup::Hit:
            node->pixmap = pixmap;
            break;
        case ImageCache::Lookup::Failed:
            break;  // 失败表命中：跳过加载，pixmap 保持为空（静默空白）
        case ImageCache::Lookup::Miss: {
            QPixmap loaded(path);
            if (loaded.isNull()) {
                Diagnostics::reportRuntime(ErrorCode::ImageLoadFailed, path,
                                           QStringLiteral("ImageElement: failed to load image"));
                ImageCache::storeFailed(path);
            } else {
                ImageCache::storeHit(path, loaded);
                node->pixmap = loaded;
            }
            break;
        }
        }
    }
    return node;
}

/// @brief 测量图像元素：显式尺寸优先，否则按 pixmap 原始尺寸自计算。
///
/// 单尺寸指定时另一维按 pixmap 宽高比推导（pixmap 为空或退化时推导为 0）；
/// 无显式尺寸时取 pixmap 原始尺寸（空 pixmap → 0×0）；最后叠加盒模型装饰。
///
/// @param ctx 布局上下文（未使用）。
/// @param constraints 可用宽高约束（未使用；自计算不依赖约束）。
/// @param node 实例节点（ImageNode）。
/// @return 含盒模型装饰的测量尺寸。
MeasureResult ImageElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    Q_UNUSED(ctx)
    Q_UNUSED(constraints)
    const auto& imageNode = static_cast<const ImageNode&>(node);
    const QPixmap& pixmap = imageNode.pixmap;

    const double pw = pixmap.isNull() ? 0.0 : pixmap.width();
    const double ph = pixmap.isNull() ? 0.0 : pixmap.height();

    double w = 0;
    double h = 0;
    if (node.style.width && node.style.height) {
        // 双显式：内容区即显式尺寸。
        w = *node.style.width;
        h = *node.style.height;
    } else if (node.style.width) {
        w = *node.style.width;
        h = (pw > 0) ? w * ph / pw : 0.0;
    } else if (node.style.height) {
        h = *node.style.height;
        w = (ph > 0) ? h * pw / ph : 0.0;
    } else {
        w = pw;
        h = ph;
    }

    w += boxModelWidth(node.style);
    h += boxModelHeight(node.style);
    return MeasureResult{QSizeF(w, h)};
}

/// @brief 存储分配的矩形（无额外缓存需求，基线即可）。
/// @param ctx 布局上下文（未使用）。
/// @param rect 分配给此图像元素的矩形。
/// @param node 实例节点（ImageNode）。
void ImageElement::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    Q_UNUSED(ctx)
    node.rect = rect;
}

/// @brief 渲染图像元素：盒模型装饰，然后在内容区内绘制 pixmap。
///
/// keepAspect=true：等比缩放至内容区内适配（contain）并两轴居中；
/// keepAspect=false：拉伸填满内容区。永不裁剪、不使用 QPainter 变换——
/// drawPixmap(targetRect, pixmap, sourceRect) 重载已足够。
///
/// @param painter 目标 QPainter。
/// @param ctx 布局上下文（未使用；图像已在物化时加载）。
/// @param node 实例节点（ImageNode）。
void ImageElement::render(QPainter* painter, const LayoutContext& ctx, const Node& node) const
{
    Q_UNUSED(ctx)
    renderBoxModel(painter, node.rect, node.style);

    const auto& imageNode = static_cast<const ImageNode&>(node);
    if (imageNode.pixmap.isNull())
        return;

    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    const QRectF content = contentRect(node.rect, node.style);
    QRectF target = content;
    if (imageNode.keepAspect && content.width() > 0 && content.height() > 0) {
        const QSizeF scaled = imageNode.pixmap.size().scaled(
            content.size().toSize(), Qt::KeepAspectRatio);
        target = QRectF(
            content.x() + (content.width() - scaled.width()) / 2.0,
            content.y() + (content.height() - scaled.height()) / 2.0,
            scaled.width(), scaled.height());
    }
    painter->drawPixmap(target, imageNode.pixmap, QRectF(imageNode.pixmap.rect()));

    painter->restore();
}

} // namespace BroadItem
