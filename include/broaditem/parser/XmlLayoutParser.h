#pragma once

#include <QString>
#include <memory>
#include <broaditem/context/LayoutContext.h>

class QDomElement;

namespace BroadItem {

class Element;
class ErrorCollector;
using ElementPtr = std::shared_ptr<Element>;

/// @brief 将 XML 布局定义解析为 Element 树。
///
/// 诊断经 ErrorCollector 单一口径投递（见 doc/设计.md「诊断」节）：
/// 返回 nullptr ⇔ 发生了至少一个 Abort 级错误（整文件失败，不产出部分树）。
class XmlLayoutParser {
public:
    /// @brief 从 XML 文件解析布局，返回根元素。
    /// @param filePath  XML 文件路径。
    /// @param collector 本次调用专用诊断收集器（可空，叠加于进程级）。
    /// @return 根元素；发生 Abort 级错误返回 nullptr。
    static ElementPtr parseFile(const QString& filePath, ErrorCollector* collector = nullptr);
    /// @brief 从 XML 字符串解析布局，返回根元素。
    /// @param xmlContent XML 文本。
    /// @param collector  本次调用专用诊断收集器（可空，叠加于进程级）。
    /// @return 根元素；发生 Abort 级错误返回 nullptr。
    static ElementPtr parseString(const QString& xmlContent, ErrorCollector* collector = nullptr);

private:
    /// @brief 在已建立的解析会话下解析 XML 字符串（parseFile/parseString 共享实现）。
    static ElementPtr parseStringInternal(const QString& xmlContent, const QString& file,
                                          ErrorCollector* collector);
    /// @brief 递归地将单个 XML 节点解析为 Element。
    static ElementPtr parseNode(const QDomElement& xml);
    /// @brief 工厂方法：根据标签名称创建 Element 实例。
    static ElementPtr createElement(const QString& tagName);
};

} // namespace BroadItem
