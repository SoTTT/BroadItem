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
#include <broaditem/core/Node.h>

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

/// @brief 所有布局元素的基类（模板层）。
///
/// 模板/实例分离架构：Element 树在 parse 后不可变，可安全共享
/// （LayoutRegistry 原样返回 ElementPtr）。三阶段流水线（测量、布局、渲染）
/// 只作用于由 materialize() 生成的实例层 Node 树，每实例状态经 Node& 进出。
class Element {
public:
    virtual ~Element() = default;

    /// @brief 从 XML 元素解析属性。仅在初始化期调用。
    virtual void parse(const QDomElement& xml);

    /// @brief 物化：以给定上下文创建该元素的实例节点。
    /// 普通元素返回 1 个节点；控制元素不产生节点（ControlElement 中 qFatal）。
    /// @param ctx 布局上下文，用于解析数据绑定。
    /// @return 新创建的实例节点。
    virtual std::unique_ptr<Node> materialize(const LayoutContext& ctx) const = 0;

    /// @brief 物化子节点序列：默认实现将 materialize() 包装为单元素向量。
    /// 控制元素（for、if-has）覆盖此方法，展开为 0..N 个节点。
    /// @param ctx 布局上下文。
    /// @return 物化后的节点向量。
    virtual std::vector<std::unique_ptr<Node>> materializeChildren(const LayoutContext& ctx) const;

    /// @brief 测量阶段：计算元素的固有尺寸，状态经 node 进出。
    /// 基类默认实现为 qFatal——控制元素永不出现在实例树中。
    virtual MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const;

    /// @brief 布局阶段：在 rect 内分配最终位置和尺寸，结果写入 node。
    /// 基类默认实现为 qFatal——控制元素永不出现在实例树中。
    virtual void layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const;

    /// @brief 使用给定 painter 渲染 node 对应的实例子树。
    /// 基类默认实现为 qFatal——控制元素永不出现在实例树中。
    virtual void render(QPainter* painter, const LayoutContext& ctx, const Node& node) const;

    /// @brief 数据绑定：如果该元素使用了指定属性名，返回 true。
    virtual bool bindsProperty(const QString& name) const { Q_UNUSED(name) return false; }

    /// @brief 检查名称是否与 bindPath 匹配（精确匹配或作为点号/括号路径的前缀）。
    static bool matchesProperty(const QString& bindPath, const QString& propName);

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
    /// @brief 从字符串解析 double，失败时返回默认值。
    static double parseDouble(const QString& value, double defaultVal = 0);
    /// @brief 从字符串解析 QColor（名称或十六进制）。
    static QColor parseColor(const QString& value);
    /// @brief 从字符串解析布尔值。
    static bool parseBool(const QString& value);
};

} // namespace BroadItem
