#pragma once

#include <QString>
#include <broaditem/expression/Expression.h>

namespace BroadItem {

/// @brief 将 XML 属性名与数据绑定路径关联的绑定对象。
///
/// 将属性名（如 ":content"、":prop"、":of"）与 Expression 路径关联，
/// 并提供检查路径是否绑定指定属性的方法。
class Binding {
public:
    /// @brief 用属性名和路径字符串构造绑定。
    /// @param attributeName XML 属性名（如 ":content"）。
    /// @param path 数据绑定路径（如 "user.name"）。
    Binding(const QString& attributeName, const QString& path);

    /// @brief 返回属性名（如 ":content"、":prop"、":of"）。
    const QString& attributeName() const { return m_attributeName; }

    /// @brief 返回底层的 Expression 对象（用于分段访问）。
    const Expression& expression() const { return m_expression; }

    /// @brief 返回原始路径字符串。
    const QString& path() const { return m_expression.path(); }

    /// @brief 如果绑定的表达式路径有效则返回 true。
    bool isValid() const { return m_expression.isValid(); }

    /// @brief 如果此绑定的路径匹配给定属性名则返回 true。
    /// 复制 Element::matchesProperty() 的前缀语义：
    /// - 精确匹配："device" == "device"
    /// - 点号前缀："device.cpu" 以 "device." 开头
    /// - 括号前缀："items[0]" 以 "items[" 开头
    bool bindsProperty(const QString& propName) const;

private:
    QString m_attributeName;  ///< XML 属性名（如 ":content"）。
    Expression m_expression;  ///< 绑定的路径表达式。
};

} // namespace BroadItem
