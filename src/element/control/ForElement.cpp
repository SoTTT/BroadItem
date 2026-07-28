#include <broaditem/element/control/ForElement.h>
#include <broaditem/context/MapPropertyContext.h>
#include <broaditem/context/ItemPropertyContext.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

/// @brief 返回 ForElement 支持的 XML 属性集合。
/// @return Reference to a static set (binding attributes b:of/b:as are namespace-aware, not listed here).
const QSet<QString>& ForElement::supportedAttributes() const
{
    static const QSet<QString> attrs = {};
    return attrs;
}

/// @brief 从 XML 元素解析 b:of 和 b:as 属性。
/// @param xml The DOM element to parse.
void ForElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttributeNS(BINDING_NS, QStringLiteral("of"))) {
        QDomNode ofNode = xml.attributes().namedItemNS(BINDING_NS, QStringLiteral("of"));
        QString ofQName = ofNode.isNull() ? QStringLiteral("b:of") : ofNode.nodeName();
        m_binding = Binding(ofQName, xml.attributeNS(BINDING_NS, QStringLiteral("of"), QString()));
    }
    QString asVal = xml.attributeNS(BINDING_NS, QStringLiteral("as"), QString());
    if (!asVal.isEmpty())
        m_asVariable = asVal;
}

/// @brief 设置提供迭代列表的绑定属性。
/// @param bind The property name for the data source (QStringList).
void ForElement::setBindProperty(const QString& bind)
{
        m_binding = Binding("b:of", bind);
}

/// @brief 设置每次迭代要物化的模板元素。
/// @param templ The template element.
void ForElement::setTemplate(ElementPtr templ)
{
    m_template = std::move(templ);
}

/// @brief 通过遍历绑定的列表将此控制元素物化为实例节点序列。
///
/// 支持 b:of 为 QStringList 或 QVariantList。为每个迭代项创建 per-item
/// MapPropertyContext（含层次和平铺键），通过 ItemPropertyContext 链到全局 context，
/// 并以 itemCtx 递归调用模板的 materializeChildren() 后拼接——嵌套 for 由此
/// 能拿到外层的 as 变量。
///
/// @param ctx The layout context providing the data property.
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
            ItemPropertyContext chainedCtx(itemCtx.get(), ctx.ctx, m_asVariable);
            LayoutContext itemLayoutCtx{&chainedCtx};

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
            ItemPropertyContext chainedCtx(itemCtx.get(), ctx.ctx, m_asVariable);
            LayoutContext itemLayoutCtx{&chainedCtx};

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
/// @param name The property name to check.
/// @return True if the property is bound.
bool ForElement::bindsProperty(const QString& name) const
{
    // 1. Check if the b:of data source property changed → always triggers relayout
    if (m_binding.bindsProperty(name))
        return true;

    // 2. Check template bindings, but strip b:as prefix — per-item variables
    //    are not global properties and should not trigger relayout.
    if (m_template) {
        if (!m_asVariable.isEmpty()) {
            // The b:as variable itself (e.g. "item") and per-item paths
            // (e.g. "item.name") are NOT global — do not trigger relayout
            QString prefix = m_asVariable + ".";
            if (name == m_asVariable || name.startsWith(prefix))
                return false;
        }
        return m_template->bindsProperty(name);
    }

    return false;
}

} // namespace BroadItem
