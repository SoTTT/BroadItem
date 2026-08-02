#pragma once

/// @file binding_helpers.h
/// @brief 测试共享辅助：布局片段包裹、解析与物化（原散落于各测试文件的 static 副本）。

#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/element/Element.h>
#include <broaditem/core/Node.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/MapPropertyContext.h>

#include <QString>
#include <memory>

namespace BroadItem {
namespace TestHelpers {

/// @brief 包裹 root 与绑定命名空间声明。
/// @param inner 根元素内部的 XML 片段。
/// @return 包裹后的完整布局文档字符串。
inline QString wrap(const QString& inner)
{
    return QStringLiteral("<root xmlns:b=\"urn:broaditem:binding\">") + inner
           + QStringLiteral("</root>");
}

/// @brief 解析单个子元素的布局片段，自动包裹 <root xmlns:b> 头尾。
/// @param xml 根元素唯一子元素的 XML 片段（可引用 b: 前缀）。
/// @return 解析后的模板元素（即唯一子元素）；失败返回 nullptr。
inline ElementPtr parse(const QString& xml)
{
    return XmlLayoutParser::parseString(wrap(xml));
}

/// @brief 以 map 为属性上下文物化 root，返回唯一实例节点。
/// @param root 模板元素（普通元素，物化产出 1 个节点）。
/// @param map  属性上下文（调用方持有，可继续 setProperty 后重新物化）。
/// @return 物化产出的第一个节点；失败返回 nullptr。
inline std::unique_ptr<Node> materializeFirst(const ElementPtr& root, MapPropertyContext& map)
{
    LayoutContext ctx;
    ctx.ctx = &map;
    auto nodes = root->materializeChildren(ctx);
    if (nodes.empty())
        return nullptr;
    return std::move(nodes[0]);
}

} // namespace TestHelpers
} // namespace BroadItem
