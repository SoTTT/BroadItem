#pragma once

#include "RenderableElement.h"
#include <QDomElement>

namespace BroadItem {

/// @brief 支持显式宽度/高度属性的元素基类。
class SizedElement : public RenderableElement {
public:
    virtual ~SizedElement() = default;

    /// @brief 从 XML 解析宽度和高度属性。
    void parse(const QDomElement& xml) override;

    /// @brief 返回显式设置的宽度（-1 表示未设置）。
    double width() const { return m_width; }
    /// @brief 返回显式设置的高度（-1 表示未设置）。
    double height() const { return m_height; }
    /// @brief 如果显式指定了宽度则返回 true。
    bool hasWidth() const { return m_width >= 0; }
    /// @brief 如果显式指定了高度则返回 true。
    bool hasHeight() const { return m_height >= 0; }

    const QSet<QString>& supportedAttributes() const override {
        static const QSet<QString> attrs = {"width", "height"};
        return attrs;
    }

protected:
    double m_width = -1;   ///< Explicit width in pixels, -1 means not specified.
    double m_height = -1;  ///< Explicit height in pixels, -1 means not specified.
};

} // namespace BroadItem
