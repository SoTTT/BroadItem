#pragma once

/// @file layout_helpers.h
/// @brief 测试共享辅助：解析、物化、测量、布局一体化（原散落于各测试文件的 static 副本）。

#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/core/LayoutEngine.h>
#include <broaditem/core/Node.h>
#include <broaditem/element/Element.h>
#include <broaditem/compat/QtCompat.h>

#include <QRectF>
#include <memory>

namespace BroadItem {
namespace TestHelpers {

/// @brief 一次性格式的布局结果包：模板树 + 属性上下文 + 物化实例节点树。
///
/// 模板/实例分离架构下，Node::element 为指向模板的非拥有指针，
/// 因此模板树（root）必须与节点树（node）同生命周期持有。
struct LayoutResult {
    ElementPtr root;              ///< 模板树（持有以保 Node::element 指针有效）。
    MapPropertyContext propCtx;   ///< 属性上下文（LayoutContext 指向它）。
    std::unique_ptr<Node> node;   ///< 物化后的实例节点树根（控制元素已展开）。
};

/// @brief 解析、物化、测量、布局一体化辅助。
/// @param xmlStr XML 布局字符串（完整文档）。
/// @param layoutRect 布局矩形；传入 null 矩形时以测量结果作为布局矩形。
/// @return 布局结果包；解析或物化失败时返回 nullptr。
inline std::unique_ptr<LayoutResult> parseAndLayout(const QString& xmlStr, QRectF layoutRect = QRectF(0, 0, 300, 200))
{
    auto result = makeUnique<LayoutResult>();
    result->root = XmlLayoutParser::parseString(xmlStr);
    if (!result->root)
        return nullptr;

    LayoutContext lctx{&result->propCtx};
    result->node = LayoutEngine::materialize(result->root, lctx);
    if (!result->node)
        return nullptr;

    const QSizeF measured = LayoutEngine::measure(lctx, LayoutConstraints{}, *result->node);
    if (layoutRect.isNull())
        layoutRect = QRectF(0, 0, measured.width(), measured.height());
    LayoutEngine::layout(lctx, layoutRect, *result->node);
    return result;
}

} // namespace TestHelpers
} // namespace BroadItem
