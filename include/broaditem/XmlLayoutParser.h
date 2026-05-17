#pragma once

#include <QString>
#include <memory>
#include "LayoutContext.h"

class QDomElement;

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

/// @brief 将 XML 布局定义解析为 Element 树。
class XmlLayoutParser {
public:
    /// @brief 从 XML 文件解析布局，返回根元素。
    static ElementPtr parseFile(const QString& filePath);
    /// @brief 从 XML 字符串解析布局，返回根元素。
    static ElementPtr parseString(const QString& xmlContent);

private:
    /// @brief 递归地将单个 XML 节点解析为 Element。
    static ElementPtr parseNode(const QDomElement& xml);
    /// @brief 工厂方法：根据标签名称创建 Element 实例。
    static ElementPtr createElement(const QString& tagName);
};

} // namespace BroadItem
