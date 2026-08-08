#pragma once

#include <QString>
#include <vector>

#include <broaditem/compat/Variant.h>

namespace BroadItem {

/// @brief 路径表达式的单个分段：对象键或数组索引的和类型（真和类型，非法状态不可表达）。
///
/// 键与索引互斥，存储为 vendored Variant<QString, int>；
/// 仅能通过 makeKey/makeIndex 工厂构造。
class PathSegment {
public:
    /// @brief 构造键分段。
    /// @param k 对象键。
    static PathSegment makeKey(const QString& k) { return PathSegment(Variant<QString, int>(k)); }
    /// @brief 构造索引分段。
    /// @param i 数组索引（非负）。
    static PathSegment makeIndex(int i) { return PathSegment(Variant<QString, int>(i)); }

    /// @brief 是否为索引分段。
    bool isIndex() const { return m_value.index() == 1; }
    /// @brief 对象键（仅键分段可调用，错误分支抛 bad_variant_access）。
    const QString& key() const { return detail::mpark::get<QString>(m_value); }
    /// @brief 数组索引（仅索引分段可调用，错误分支抛 bad_variant_access）。
    int index() const { return detail::mpark::get<int>(m_value); }

private:
    explicit PathSegment(const Variant<QString, int>& v) : m_value(v) {}

    Variant<QString, int> m_value;  ///< 分段值：键（备选 0）或索引（备选 1）。
};

/// @brief 路径语法的唯一权威：点号和括号路径表达式解析器，将路径解析为类型化分段序列。
///
/// 将 "device.cpu" 解析为 [key:device, key:cpu]，
/// 将 "items[0].name" 解析为 [key:items, index:0, key:name]。
/// 运行期路径遍历（PropertyContext）一律消费本类产出的分段，
/// 不再各自解释路径字符串——语法只在本类定义一次。
///
/// 语法规则（任意一条违反即 invalid）：
/// - 键分段为 Unicode 标识符：首字符字母（含中文等）或 '_'，后续允许字母/数字/'_'；
/// - 路径不得为空、不得以 '.' 或 '[' 开头、不得含连续 '.'；
/// - '[' 必须紧跟键或 ']'（即允许 a[0][1] 连续索引）；
/// - 索引必须为非负整数（容忍 QString::toInt 的空白/前导正号）；
/// - ']' 之后只允许 '.'、'[' 或结尾（a[0]b 非法）；
/// - 不得以 '.' 结尾（a. 与 a[0]. 均非法）。
class Expression {
public:
    /// @brief 用原始路径字符串构造表达式并解析分段。
    explicit Expression(const QString& path);

    /// @brief 返回原始路径字符串。
    const QString& path() const { return m_path; }

    /// @brief 如果路径解析成功则返回 true。
    bool isValid() const { return m_valid; }

    /// @brief 返回解析后的类型化路径分段。
    /// 示例："device.cpu" → [key:device, key:cpu]
    /// 示例："items[0].name" → [key:items, index:0, key:name]
    const std::vector<PathSegment>& segments() const { return m_segments; }

    /// @brief 路径前缀匹配：bindPath 是否以 propName 为首段。
    /// 规则：精确相等，或以 "propName." / "propName[" 开头。
    /// 为 Element::matchesProperty 与 Binding::bindsProperty 的唯一实现。
    /// @param bindPath 绑定路径字符串。
    /// @param propName 要匹配的属性名。
    /// @return 首段匹配返回 true。
    static bool pathMatches(const QString& bindPath, const QString& propName);

private:
    QString m_path;              ///< 原始路径字符串。
    bool m_valid = false;        ///< 解析是否成功。
    std::vector<PathSegment> m_segments; ///< 解析后的类型化分段。

    /// @brief 解析路径字符串为分段向量。
    void parse();
};

} // namespace BroadItem
