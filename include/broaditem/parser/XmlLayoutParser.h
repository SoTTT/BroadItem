#pragma once

#include <QString>
#include <memory>

class QDomElement;
class QIODevice;

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
    /// @brief 从已打开的 QIODevice 解析布局，返回根元素。
    ///
    /// Qt I/O 抽象入口：QFile（含 qrc ":/" 路径）、QBuffer（内存字符串）等
    /// 一切 QIODevice 子类均可。沿袭 QDomDocument::setContent 惯例——
    /// 调用方持有设备并负责打开，本函数只读全部内容后解析。
    /// 诊断来源标签：设备为 QFile 时取其 fileName()，否则用 "<device>"。
    /// @param device    已以可读模式打开的设备（不可读时按 BI-P-001 报告）。
    /// @param collector 本次调用专用诊断收集器（可空，叠加于进程级）。
    /// @return 根元素；发生 Abort 级错误返回 nullptr。
    static ElementPtr parseDevice(QIODevice* device, ErrorCollector* collector = nullptr);

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
