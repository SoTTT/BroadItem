#include <broaditem/element/control/IfElement.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/compat/QtCompat.h>
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 IfElement 支持的 XML 属性集合。
/// @return 静态集合 {"not", "equals"}（b:prop/b:equals 走绑定命名空间，不在此列出）。
const QSet<QString>& IfElement::supportedAttributes() const
{
    static const QSet<QString> attrs = {"not", "equals"};
    return attrs;
}

/// @brief 声明有求值路径的绑定属性：b:equals 在 shouldShow 中经 boundValue 求值。
/// @return 静态集合 {"equals"}。
const QSet<QString>& IfElement::resolvedAttributes() const
{
    static const QSet<QString> attrs = {"equals"};
    return attrs;
}

/// @brief 从 XML 元素解析 b:prop、not 与 equals 属性。
/// @param xml The DOM element to parse.
void IfElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttributeNS(BINDING_NS, "prop")) {
        /// 获取实际的 XML 限定属性名（如 b:prop 或用户自定义前缀）
        QDomNode attrNode = xml.attributes().namedItemNS(BINDING_NS, "prop");
        QString attrName = attrNode.isNull() ? "b:prop" : attrNode.nodeName();
        m_binding = Binding(attrName, xml.attributeNS(BINDING_NS, "prop", QString()));
    }
    // QDom 命名空间模式下 hasAttribute 按局部名匹配（会命中 b:not 自身），字面量判定走 hasLiteralAttribute
    m_not = hasLiteralAttribute(xml, "not");
    // not 是结构性修饰符，不参与绑定；parseBindings 已将 not 列为保留名，此处显式报告
    if (xml.hasAttributeNS(BINDING_NS, "not"))
        Diagnostics::reportParse(ErrorCode::NotBindingIgnored,
                                 QStringLiteral("<if>: b:not is not supported; not is a structural modifier "
                                                "and cannot be bound"));
    if (hasLiteralAttribute(xml, "equals")) {
        m_equals = literalAttribute(xml, "equals");
        m_hasEquals = true;
    }
}

/// @brief 设置条件作用的属性路径。
/// @param bind The property path.
void IfElement::setBindProperty(const QString& bind)
{
    m_binding = Binding("b:prop", bind);
}

/// @brief 设置条件是否整体取反。
/// @param notValue If true, the element is shown when the condition does NOT hold.
void IfElement::setNot(bool notValue)
{
    m_not = notValue;
}

/// @brief 设置字面量比较值。
/// @param equals 比较目标字符串（可为空串，与未指定语义不同）。
void IfElement::setEquals(const QString& equals)
{
    m_equals = equals;
    m_hasEquals = true;
}

/// @brief 设置要条件显示的子元素。
/// @param child The child element.
void IfElement::setChild(ElementPtr child)
{
    m_child = std::move(child);
}

/// @brief 评估条件：存在性（裸 b:prop）或值比较（equals 字面量 / b:equals 绑定），not 整体取反。
///
/// 值比较为字符串化比较：两侧取 QVariant::toString() 后区分大小写比较。
/// 绑定比较值不可解析（路径不存在或绑定无效）时该次求值条件不成立。
///
/// @param ctx The layout context to query.
/// @return True if the child should be shown (respecting the "not" flag).
bool IfElement::shouldShow(const LayoutContext& ctx) const
{
    bool base = ctx.hasProperty(m_binding.path());
    QVariant value;
    if (base) {
        value = ctx.property(m_binding.path());
        // variantIsNull 统一 Qt5/Qt6 的 null 语义：null QString 在两版下均视为不存在
        base = !variantIsNull(value);
    }

    bool cond = base;
    if (m_hasEquals) {
        // 字面量比较值
        cond = base && value.toString() == m_equals;
    } else if (const Binding* eq = bindingFor("equals")) {
        // 绑定比较值：不可解析时条件不成立（数据未就绪时隐藏，避免闪现错误内容）
        QVariant target = boundValue("equals", ctx);
        cond = base && target.isValid() && value.toString() == target.toString();
    }
    return m_not ? !cond : cond;
}

/// @brief 条件满足时物化子元素，否则返回空向量。
/// @param ctx The layout context.
/// @return 子元素的物化节点序列，或空。
std::vector<std::unique_ptr<Node>> IfElement::materializeChildren(const LayoutContext& ctx) const
{
    if (!shouldShow(ctx) || !m_child)
        return {};
    return m_child->materializeChildren(ctx);
}

/// @brief 检查此元素是否绑定指定属性（b:prop 路径、b:equals 等通用绑定、或子元素内）。
/// @param name The property name to check.
/// @return True if the property is bound.
bool IfElement::bindsProperty(const QString& name) const
{
    return m_binding.bindsProperty(name) || Element::bindsProperty(name)
           || (m_child && m_child->bindsProperty(name));
}

} // namespace BroadItem
