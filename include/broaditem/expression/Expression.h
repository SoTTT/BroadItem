#pragma once

#include <QString>
#include <vector>

namespace BroadItem {

class Expression {
public:
    explicit Expression(const QString& path);

    /// @brief Returns the original path string.
    const QString& path() const { return m_path; }

    /// @brief Returns true if the path parsed successfully.
    bool isValid() const { return m_valid; }

    /// @brief Returns parsed path segments.
    /// Example: "device.cpu" → ["device", "cpu"]
    /// Example: "items[0].name" → ["items", "0", "name"]
    const std::vector<QString>& segments() const { return m_segments; }

private:
    QString m_path;
    bool m_valid = false;
    std::vector<QString> m_segments;

    void parse();
};

} // namespace BroadItem
