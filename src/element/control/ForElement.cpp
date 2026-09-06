#include <broaditem/element/control/ForElement.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 ForElement 支持的 XML 属性集合。
/// @return 静态集合引用（绑定属性 b:of/b:as 走命名空间，不在此列出）。
const QSet<QString>& ForElement::supportedAttributes() const
{
    static const QSet<QString> attrs = {};
    return attrs;
}

/// @brief 从 XML 元素解析 b:of 和 b:as 属性。
/// @param xml 要解析的 DOM 元素。
void ForElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttributeNS(BINDING_NS, BindingName::Of)) {
        QDomNode ofNode = xml.attributes().namedItemNS(BINDING_NS, BindingName::Of);
        QString ofQName = ofNode.isNull() ? QStringLiteral("b:of") : ofNode.nodeName();
        m_binding = Binding(ofQName, xml.attributeNS(BINDING_NS, BindingName::Of, QString()));
    }
    QString asVal = xml.attributeNS(BINDING_NS, BindingName::As, QString());
    if (!asVal.isEmpty())
        m_asVariable = asVal;
}

/// @brief 设置提供迭代列表的绑定属性。
/// @param bind 数据源的属性名（QStringList）。
void ForElement::setBindProperty(const QString& bind)
{
    m_binding = Binding("b:of", bind);
}

/// @brief 设置每次迭代要物化的模板元素。
/// @param templ 模板元素。
void ForElement::setTemplate(ElementPtr templ)
{
    m_template = std::move(templ);
}

/// @brief 解析期挂载：保留第一个子元素作为迭代模板，多余忽略（现状行为）。
/// @param child 解析完成的子元素。
void ForElement::addParsedChild(const ElementPtr& child)
{
    if (!m_template)
        setTemplate(child);
}

/// @brief 通过遍历绑定的列表将此控制元素物化为实例节点序列。
///
/// 支持 b:of 为 QStringList 或 QVariantList。为每个迭代项创建 per-item
/// MapPropertyContext（含层次和平铺键），通过 LayoutContext::Scope 栈链挂接
/// per-item 作用域后递归调用模板的 materializeChildren() 再拼接——嵌套 for
/// 由此沿 parent 链拿到外层的 as 变量。
///
/// @param ctx 提供数据属性的布局上下文。
/// @return 物化后的实例节点向量。
std::vector<std::unique_ptr<Node>> ForElement::materializeChildren(const LayoutContext& ctx) const
{
    std::vector<std::unique_ptr<Node>> result;
    if (!m_template || m_binding.path().isEmpty())
        return result;

    if (m_asVariable.isEmpty()) {
        Diagnostics::reportRuntime(ErrorCode::ForMissingAs, m_binding.path(),
                                   QStringLiteral("ForElement: b:as attribute is required, but not set"));
        return result;
    }

    QVariant v = ctx.property(m_binding.path());
    if (!v.isValid())
        return result;

    if (v.userType() == QMetaType::QStringList) {
        QStringList values = v.toStringList();
        for (const auto& val : values) {
            auto itemCtx = std::make_shared<MapPropertyContext>();
            itemCtx->setProperty(m_asVariable, val);
            LayoutContext::Scope scopeNode{itemCtx.get(), m_asVariable, ctx.scope};
            LayoutContext itemLayoutCtx = ctx;
            itemLayoutCtx.scope = &scopeNode;

            auto nodes = m_template->materializeChildren(itemLayoutCtx);
            for (auto& n : nodes)
                result.push_back(std::move(n));
        }
    } else if (v.userType() == QMetaType::QVariantList) {
        QVariantList list = v.toList();
        for (const auto& val : list) {
            if (!val.isValid()) {
                Diagnostics::reportRuntime(ErrorCode::ForNullItemSkipped, m_binding.path(),
                                           QStringLiteral("ForElement: skipping null item in list"));
                continue;
            }
            auto itemCtx = std::make_shared<MapPropertyContext>();
            itemCtx->setProperty(m_asVariable, val);
            if (val.userType() == QMetaType::QVariantMap) {
                QVariantMap map = val.toMap();
                for (auto it = map.begin(); it != map.end(); ++it)
                    itemCtx->setProperty(it.key(), it.value());
            }
            LayoutContext::Scope scopeNode{itemCtx.get(), m_asVariable, ctx.scope};
            LayoutContext itemLayoutCtx = ctx;
            itemLayoutCtx.scope = &scopeNode;

            auto nodes = m_template->materializeChildren(itemLayoutCtx);
            for (auto& n : nodes)
                result.push_back(std::move(n));
        }
    } else {
        Diagnostics::reportRuntime(ErrorCode::ForDataNotList, m_binding.path(),
                                   QStringLiteral("ForElement: property expected QStringList or QVariantList, "
                                                  "got %1").arg(QLatin1String(v.typeName())));
    }

    return result;
}

/// @brief 检查此元素是否绑定指定属性（通过 b:of 或在模板中）。
/// @param name 要检查的属性名。
/// @return 属性被绑定时返回 true。
bool ForElement::bindsProperty(const QString& name) const
{
    // 1. 检查 b:of 数据源属性是否变更 → 命中即触发重布局
    if (m_binding.bindsProperty(name))
        return true;

    // 2. 检查模板内的绑定，但剥离 b:as 前缀——迭代项变量
    //    不是全局属性，不应触发重布局。
    if (m_template) {
        if (!m_asVariable.isEmpty()) {
            // b:as 变量本身（如 "item"）与迭代项路径（如 "item.name"）
            // 都不是全局属性——不触发重布局
            QString prefix = m_asVariable + ".";
            if (name == m_asVariable || name.startsWith(prefix))
                return false;
        }
        return m_template->bindsProperty(name);
    }

    return false;
}

} // namespace BroadItem
