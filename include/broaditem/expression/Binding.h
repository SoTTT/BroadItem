#pragma once

#include <QString>
#include <broaditem/expression/Expression.h>

namespace BroadItem {

class Binding {
public:
    Binding(const QString& attributeName, const QString& path);

    /// @brief Returns the attribute name (e.g., ":content", ":prop", ":of").
    const QString& attributeName() const { return m_attributeName; }

    /// @brief Returns the underlying Expression (for segment access).
    const Expression& expression() const { return m_expression; }

    /// @brief Returns the original path string.
    const QString& path() const { return m_expression.path(); }

    /// @brief Returns true if the binding's expression path is valid.
    bool isValid() const { return m_expression.isValid(); }

    /// @brief Returns true if this binding's path matches the given property name.
    /// Replicates Element::matchesProperty() prefix semantics:
    /// - Exact match: "device" == "device"
    /// - Dot prefix: "device.cpu" starts with "device."
    /// - Bracket prefix: "items[0]" starts with "items["
    bool bindsProperty(const QString& propName) const;

private:
    QString m_attributeName;
    Expression m_expression;
};

} // namespace BroadItem
