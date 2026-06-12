#pragma once

#include <QString>
#include <vector>

namespace BroadItem {

/// @brief 点号和括号路径表达式解析器，将路径解析为分段向量。
///
/// 将 "device.cpu" 解析为 ["device", "cpu"]，
/// 将 "items[0].name" 解析为 ["items", "0", "name"]。
class Expression {
public:
    /// @brief 用原始路径字符串构造表达式并解析分段。
    explicit Expression(const QString& path);

    /// @brief 返回原始路径字符串。
    const QString& path() const { return m_path; }

    /// @brief 如果路径解析成功则返回 true。
    bool isValid() const { return m_valid; }

    /// @brief 返回解析后的路径分段。
    /// 示例："device.cpu" → ["device", "cpu"]
    /// 示例："items[0].name" → ["items", "0", "name"]
    const std::vector<QString>& segments() const { return m_segments; }

private:
    QString m_path;              ///< 原始路径字符串。
    bool m_valid = false;        ///< 解析是否成功。
    std::vector<QString> m_segments; ///< 解析后的路径分段。

    /// @brief 解析路径字符串为分段向量。
    void parse();
};

} // namespace BroadItem
