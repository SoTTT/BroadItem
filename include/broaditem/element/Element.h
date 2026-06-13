#pragma once

#include <QPainter>
#include <QDomElement>
#include <QDomAttr>
#include <QStringList>
#include <QSet>
#include <QRectF>
#include <memory>
#include <vector>
#include <broaditem/context/LayoutContext.h>

namespace BroadItem {

/// @brief 绑定属性的 XML 命名空间 URI。
extern const QString BINDING_NS;

/// @brief 判断属性是否为 xmlns 命名空间声明。
/// @param attr The DOM attribute to check.
/// @return True if the attribute name starts with "xmlns".
bool isNamespaceDeclaration(const QDomAttr& attr);

/// @brief 判断属性是否属于绑定命名空间。
/// @param attr The DOM attribute to check.
/// @return True if attr.namespaceURI() == BINDING_NS.
bool isBindingAttribute(const QDomAttr& attr);

class Element;
using ElementPtr = std::shared_ptr<Element>;

/// @brief 所有布局元素的基类，用于三阶段流水线（测量、布局、渲染）。
class Element {
public:
    virtual ~Element() = default;

    /// @brief 从 XML 元素解析属性。
    virtual void parse(const QDomElement& xml);

    /// @brief 测量阶段：计算元素的固有尺寸。
    virtual MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) = 0;

    /// @brief 布局阶段：分配最终位置和尺寸。
    virtual void layout(const LayoutContext& ctx, const QRectF& rect) = 0;

    /// @brief 使用给定 painter 渲染该元素。
    virtual void render(QPainter* painter, const LayoutContext& ctx) const = 0;

    /// @brief 数据绑定：如果该元素使用了指定属性名，返回 true。
    virtual bool bindsProperty(const QString& name) const { Q_UNUSED(name) return false; }

    /// @brief 检查名称是否与 bindPath 匹配（精确匹配或作为点号/括号路径的前缀）。
    static bool matchesProperty(const QString& bindPath, const QString& propName);

    /// @brief 克隆该元素（深拷贝）。所有具体元素类型必须实现。
    virtual ElementPtr clone() const = 0;

    /// @brief 根据给定 context 解析并绑定属性值。expand() 对每个克隆调用此方法。
    virtual void resolveBindings(const LayoutContext& ctx);

    /// @brief 返回该元素的包围矩形。
    const QRectF& rect() const { return m_rect; }
    /// @brief 设置该元素的包围矩形。
    void setRect(const QRectF& r) { m_rect = r; }

    /// @brief 返回该元素支持的属性名集合。
    virtual const QSet<QString>& supportedAttributes() const {
        static const QSet<QString> empty;
        return empty;
    }

    /// @brief 如果该元素类型可以有子元素，返回 true。
    virtual bool canHaveChildren() const { return false; }

    /// @brief 根据 supportedAttributes 验证 XML 元素中的属性。
    void validateAttributes(const QDomElement& xml) const;

    /// @brief 验证属性值是否为 double，有效返回 true。
    static bool validateDouble(const QString& value, const QString& attrName, double& out);
    /// @brief 验证属性值是否为 int，有效返回 true。
    static bool validateInt(const QString& value, const QString& attrName, int& out);
    /// @brief 验证属性值是否为 bool，有效返回 true。
    static bool validateBool(const QString& value, const QString& attrName, bool& out);

    /// @brief BroadItem 数据绑定属性的 XML 命名空间 URI。
    inline static const QString BINDING_NS = QStringLiteral("urn:broaditem:binding");

    /// @brief 判断属性是否属于 BINDING_NS 命名空间。
    /// @param attr A DOM attribute node.
    /// @return True if attr.namespaceURI() == BINDING_NS.
    static bool isBindingAttribute(const QDomAttr& attr)
    {
        return attr.namespaceURI() == BINDING_NS;
    }

    /// @brief 判断属性是否为命名空间声明（xmlns 或 xmlns:prefix）。
    /// @param attr A DOM attribute node.
    /// @return True if attr.name() starts with "xmlns".
    static bool isNamespaceDeclaration(const QDomAttr& attr)
    {
        return attr.name().startsWith(QLatin1String("xmlns"));
    }

protected:
    QRectF m_rect;  ///< The bounding rectangle of this element.

    /// @brief 从字符串解析 double，失败时返回默认值。
    static double parseDouble(const QString& value, double defaultVal = 0);
    /// @brief 从字符串解析 QColor（名称或十六进制）。
    static QColor parseColor(const QString& value);
    /// @brief 从字符串解析布尔值。
    static bool parseBool(const QString& value);
};

} // namespace BroadItem
