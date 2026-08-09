#include <broaditem/element/Element.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <QDomElement>
#include <QtGlobal>

namespace BroadItem {

/// @brief 绑定属性的 XML 命名空间 URI。
const QString BINDING_NS = QStringLiteral("urn:broaditem:binding");

// 保留绑定名常量定义（C++11 无 inline 变量，定义收口在本翻译单元）。
const QString BindingName::Of      = QStringLiteral("of");
const QString BindingName::Prop    = QStringLiteral("prop");
const QString BindingName::As      = QStringLiteral("as");
const QString BindingName::Content = QStringLiteral("content");
const QString BindingName::Not     = QStringLiteral("not");

const QSet<QString>& reservedBindingNames()
{
    static const QSet<QString> names{BindingName::Of, BindingName::Prop, BindingName::As,
                                     BindingName::Content, BindingName::Not};
    return names;
}

/// @brief 判断属性是否为 xmlns 命名空间声明。
/// @param attr 要检查的 DOM 属性。
/// @return 属性名以 "xmlns" 开头时返回 true。
bool isNamespaceDeclaration(const QDomAttr& attr)
{
    return attr.name().startsWith(QLatin1String("xmlns"));
}

/// @brief 判断属性是否属于绑定命名空间。
/// @param attr 要检查的 DOM 属性。
/// @return attr.namespaceURI() == BINDING_NS 时返回 true。
bool isBindingAttribute(const QDomAttr& attr)
{
    return attr.namespaceURI() == BINDING_NS;
}

/// @brief 基础解析方法；子类覆盖以提取其属性。
/// @param xml 要解析的 DOM 元素。
void Element::parse(const QDomElement& xml)
{
    Q_UNUSED(xml)
}

/// @brief 默认物化子节点实现：将 materialize() 包装为单元素向量。
/// @param ctx 布局上下文。
/// @return 仅含 materialize() 结果的向量。
std::vector<std::unique_ptr<Node>> Element::materializeChildren(const LayoutContext& ctx) const
{
    std::vector<std::unique_ptr<Node>> result;
    result.push_back(materialize(ctx));
    return result;
}

/// @brief 基类测量默认实现：不可达（控制元素永不出现在实例树中）。
MeasureResult Element::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    Q_UNUSED(ctx)
    Q_UNUSED(constraints)
    Q_UNUSED(node)
    qFatal("Element::measure: control elements must not appear in node trees");
    return MeasureResult{QSizeF(0, 0)};  // unreachable
}

/// @brief 基类布局默认实现：不可达（控制元素永不出现在实例树中）。
void Element::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    Q_UNUSED(ctx)
    Q_UNUSED(rect)
    Q_UNUSED(node)
    qFatal("Element::layout: control elements must not appear in node trees");
}

/// @brief 基类渲染默认实现：不可达（控制元素永不出现在实例树中）。
void Element::render(QPainter* painter, const LayoutContext& ctx, const Node& node) const
{
    Q_UNUSED(painter)
    Q_UNUSED(ctx)
    Q_UNUSED(node)
    qFatal("Element::render: control elements must not appear in node trees");
}

/// @brief 基类基线默认实现：-1 表示无基线（调用方回退为底边对齐）。
double Element::baselineOffset(const LayoutContext& ctx, const Node& node) const
{
    Q_UNUSED(ctx)
    Q_UNUSED(node)
    return -1;
}

/// @brief 基类交叉轴填充默认实现：false 表示不请求恒填充（容器行为照旧）。
bool Element::fillsCrossAxis() const
{
    return false;
}

/// @brief 基类挂载默认实现：忽略（叶子/无关类型走不到这里，见头文件契约）。
void Element::addParsedChild(const ElementPtr& child)
{
    Q_UNUSED(child)
}

/// @brief 基类校验默认实现：通过。
bool Element::validateChildren() const
{
    return true;
}

/// @brief 安全地将字符串解析为 double。
/// @param value 要解析的字符串。
/// @param defaultVal 解析失败时的返回值。
/// @return 解析出的 double；失败时返回 defaultVal。
double Element::parseDouble(const QString& value, double defaultVal)
{
    bool ok = false;
    double result = value.toDouble(&ok);
    return ok ? result : defaultVal;
}

/// @brief 将字符串解析为 QColor。
/// @param value 颜色字符串（QColor 接受的任意格式）。
/// @return 解析出的 QColor。
QColor Element::parseColor(const QString& value)
{
    return {value};
}

/// @brief 将字符串解析为布尔值。
/// @param value "true"/"1" 返回 true，其余返回 false。
/// @return 解析出的布尔值。
bool Element::parseBool(const QString& value)
{
    return value.compare("true", Qt::CaseInsensitive) == 0 || value == "1";
}

/// @brief 检查属性名是否匹配绑定路径（支持点和括号子路径）。
/// @param bindPath 绑定路径（如 "user.name" 或 "items[0]"）。
/// @param propName 要匹配的属性名。
/// @return propName 是 bindPath 的前缀匹配时返回 true。
bool Element::matchesProperty(const QString& bindPath, const QString& propName)
{
    // 前缀匹配语义唯一实现在 Expression::pathMatches()。
    return Expression::pathMatches(bindPath, propName);
}

/// @brief 通过与 supportedAttributes() 比较来报告未知 XML 属性（BI-P-011）。
///
/// 遍历所有 XML 属性，跳过 xmlns 声明属性和绑定命名空间属性，
/// 其余属性若不在 supportedAttributes() 集合中则报告诊断。
///
/// @param xml 要校验属性的 DOM 元素。
void Element::validateAttributes(const QDomElement& xml) const
{
    const QSet<QString>& known = supportedAttributes();
    QDomNamedNodeMap attrs = xml.attributes();
    for (int i = 0; i < attrs.size(); ++i) {
        QDomAttr attr = attrs.item(i).toAttr();
        if (isNamespaceDeclaration(attr))
            continue;
        if (isBindingAttribute(attr))
            continue;
        QString name = attr.name();
        if (!known.contains(name)) {
            Diagnostics::reportParse(ErrorCode::UnknownAttribute,
                                     QStringLiteral("%1: unknown attribute %2=\"%3\"")
                                         .arg(xml.tagName(), name, attr.value()));
        }
    }
}

/// @brief 报告数值/布尔校验失败：runtimePath 为空按解析期字面量错误（BI-P-015），
/// 否则按运行时绑定值错误（BI-R-011）。
/// @param runtimePath 绑定路径（空表示解析期字面量）。
/// @param message     模板化消息。
static void reportValueMismatch(const QString& runtimePath, const QString& message)
{
    if (runtimePath.isEmpty())
        Diagnostics::reportParse(ErrorCode::LiteralTypeMismatch, message);
    else
        Diagnostics::reportRuntime(ErrorCode::BoundValueTypeError, runtimePath, message);
}

/// @brief 验证并将字符串解析为 double，失败时报告诊断。
/// @param value 要解析的字符串。
/// @param attrName 用于错误消息的属性名。
/// @param out 解析值的输出参数。
/// @param runtimePath 绑定路径（非空表示运行时绑定值求值）。
/// @return 解析成功时返回 true。
bool Element::validateDouble(const QString& value, const QString& attrName, double& out,
                             const QString& runtimePath)
{
    bool ok = false;
    const double parsed = value.toDouble(&ok);
    if (!ok) {
        reportValueMismatch(runtimePath, QStringLiteral("Attribute %1 expects a number, got \"%2\"")
                                             .arg(attrName, value));
        return false;  // 失败不写 out：属性保持默认（BI-P-015 恢复语义）
    }
    out = parsed;
    return true;
}

/// @brief 验证并将字符串解析为整数，失败时报告诊断。
/// @param value 要解析的字符串。
/// @param attrName 用于错误消息的属性名。
/// @param out 解析值的输出参数。
/// @param runtimePath 绑定路径（非空表示运行时绑定值求值）。
/// @return 解析成功时返回 true。
bool Element::validateInt(const QString& value, const QString& attrName, int& out,
                          const QString& runtimePath)
{
    bool ok = false;
    const int parsed = value.toInt(&ok);
    if (!ok) {
        reportValueMismatch(runtimePath, QStringLiteral("Attribute %1 expects an integer, got \"%2\"")
                                             .arg(attrName, value));
        return false;  // 失败不写 out：属性保持默认（BI-P-015 恢复语义）
    }
    out = parsed;
    return true;
}

/// @brief 验证并将字符串解析为布尔值（"true"/"false"/"1"/"0"），失败时报告诊断。
/// @param value 要解析的字符串。
/// @param attrName 用于错误消息的属性名。
/// @param out 解析值的输出参数。
/// @param runtimePath 绑定路径（非空表示运行时绑定值求值）。
/// @return 解析成功时返回 true。
bool Element::validateBool(const QString& value, const QString& attrName, bool& out,
                           const QString& runtimePath)
{
    QString v = value.toLower().trimmed();
    if (v == QLatin1String("true") || v == QLatin1String("1")) {
        out = true;
        return true;
    }
    if (v == QLatin1String("false") || v == QLatin1String("0")) {
        out = false;
        return true;
    }
    reportValueMismatch(runtimePath, QStringLiteral("Attribute %1 expects true/false or 1/0, got \"%2\"")
                                         .arg(attrName, value));
    return false;
}

/// @brief 基类 resolvedAttributes：返回空集，表示无通用绑定求值路径。
/// @return 空集合的静态引用。
const QSet<QString>& Element::resolvedAttributes() const
{
    static const QSet<QString> empty;
    return empty;
}

/// @brief 解析 XML 元素上的通用绑定属性（b:attr 形式）存入 m_bindings。
///
/// 保留名 {"of","prop","as","content","not"} 与控制元素/文本语义耦合，不纳入通用
/// 绑定——参见设计.md 记载的伪属性双重包装陷阱：parseNode 前置判定 wrapFor/wrapIf
/// 并在尾部包装已解析元素，若这些名字也进通用表，
/// 同一属性将被两套机制重复解析。"not" 是 <if> 的结构性修饰符（取反），
/// 如同代码中只修改变量而不修改条件表达式的取反，永不参与绑定。
///
/// @param xml 要解析绑定属性的 DOM 元素。
void Element::parseBindings(const QDomElement& xml)
{
    const QSet<QString>& reserved = reservedBindingNames();
    const QSet<QString>& known = supportedAttributes();
    QDomNamedNodeMap attrs = xml.attributes();
    for (int i = 0; i < attrs.size(); ++i) {
        QDomAttr attr = attrs.item(i).toAttr();
        if (!isBindingAttribute(attr))
            continue;
        // 命名空间处理开启时 name() 返回局部名（无前缀），nodeName() 返回限定名（如 "b:color"）。
        const QString local = attr.name();
        if (reserved.contains(local))
            continue;
        if (!known.contains(local)) {
            Diagnostics::reportParse(ErrorCode::UnknownBindingAttribute,
                                     QStringLiteral("%1: unknown binding attribute %2=\"%3\"")
                                         .arg(xml.tagName(), attr.nodeName(), attr.value()));
            continue;
        }
        // QDom 在命名空间模式下 hasAttribute/attribute/hasAttributeNS 均不可靠：
        // hasAttribute 按局部名匹配（会命中 b:color 自身），hasAttributeNS(QString(), ...)
        // 又匹配不到无命名空间字面量。只能遍历属性，要求局部名相同且 namespaceURI 为空。
        bool hasLiteral = false;
        for (int j = 0; j < attrs.size(); ++j) {
            QDomAttr a = attrs.item(j).toAttr();
            if (a.namespaceURI().isEmpty() && a.name() == local) {
                hasLiteral = true;
                break;
            }
        }
        if (hasLiteral) {
            Diagnostics::reportParse(ErrorCode::MutexLiteralBinding,
                                     QStringLiteral("%1: %2 and %3 are mutually exclusive; ignoring the binding")
                                         .arg(xml.tagName(), local, attr.nodeName()));
            continue;
        }
        // 布局策略/结构性属性不参与绑定（语义收窄，2026-07）：supported 但无求值路径
        // 的绑定在解析期拒绝（BI-P-023），忽略且不注册。
        if (!resolvedAttributes().contains(local)) {
            Diagnostics::reportParse(ErrorCode::BindingNotSupported,
                                     QStringLiteral("%1: attribute \"%2\" does not support binding "
                                                    "(layout policy); binding ignored")
                                         .arg(xml.tagName(), attr.nodeName()));
            continue;
        }
        m_bindings.insert(local, Binding(attr.nodeName(), attr.value()));
    }
}

/// @brief 按局部属性名查找通用绑定。
/// @param attribute 局部属性名（如 "color"）。
/// @return 命中的 Binding 指针；未命中返回 nullptr。
const Binding* Element::bindingFor(const QString& attribute) const
{
    auto it = m_bindings.constFind(attribute);
    return it == m_bindings.constEnd() ? nullptr : &it.value();
}

/// @brief 基类 bindsProperty：遍历通用绑定表，任一绑定匹配即返回 true。
/// @param name 要检查的属性名。
/// @return 任一通用绑定匹配该属性时返回 true。
bool Element::bindsProperty(const QString& name) const
{
    for (const Binding& b : m_bindings) {
        if (b.bindsProperty(name))
            return true;
    }
    return false;
}

/// @brief 求值通用绑定：返回绑定路径在上下文中的属性值。
/// @param attribute 局部属性名。
/// @param ctx 布局上下文。
/// @return 绑定值；无绑定/绑定无效/上下文无此属性时返回无效 QVariant。
QVariant Element::boundValue(const QString& attribute, const LayoutContext& ctx) const
{
    const Binding* b = bindingFor(attribute);
    if (!b || !b->isValid())
        return {};
    if (!ctx.hasProperty(b->path()))
        return {};  // 静默回退：属性尚未注入上下文属正常状态
    return ctx.property(b->path());
}

/// @brief 求值绑定为 double：无效绑定或转换失败返回 fallback。
double Element::resolveDouble(const QString& attribute, const LayoutContext& ctx, double fallback) const
{
    QVariant v = boundValue(attribute, ctx);
    if (!v.isValid())
        return fallback;
    double out = fallback;
    if (!validateDouble(v.toString(), bindingFor(attribute)->attributeName(), out,
                        bindingFor(attribute)->path()))
        return fallback;
    return out;
}

/// @brief 求值绑定为可空 double：无效绑定或转换失败返回可空 fallback。
Optional<double> Element::resolveDouble(const QString& attribute, const LayoutContext& ctx,
                                        const Optional<double>& fallback) const
{
    QVariant v = boundValue(attribute, ctx);
    if (!v.isValid())
        return fallback;
    double out = fallback.value_or(0);
    if (!validateDouble(v.toString(), bindingFor(attribute)->attributeName(), out,
                        bindingFor(attribute)->path()))
        return fallback;
    return out;
}

/// @brief 求值绑定为 QColor：非字符串或颜色无效时报告诊断并返回 fallback。
QColor Element::resolveColor(const QString& attribute, const LayoutContext& ctx, const QColor& fallback) const
{
    QVariant v = boundValue(attribute, ctx);
    if (!v.isValid())
        return fallback;
    const Binding* b = bindingFor(attribute);
    if (v.userType() != QMetaType::QString) {
        Diagnostics::reportRuntime(ErrorCode::BoundValueTypeError, b->path(),
                                   QStringLiteral("Attribute %1 expects a color string, got type %2")
                                       .arg(b->attributeName(), QLatin1String(v.typeName())));
        return fallback;
    }
    QColor color(v.toString());
    if (!color.isValid()) {
        Diagnostics::reportRuntime(ErrorCode::BoundValueTypeError, b->path(),
                                   QStringLiteral("Attribute %1 expects a valid color, got \"%2\"")
                                       .arg(b->attributeName(), v.toString()));
        return fallback;
    }
    return color;
}

/// @brief 求值绑定为 bool：Bool 型直接取值，否则按字符串验证，失败返回 fallback。
bool Element::resolveBool(const QString& attribute, const LayoutContext& ctx, bool fallback) const
{
    QVariant v = boundValue(attribute, ctx);
    if (!v.isValid())
        return fallback;
    if (v.userType() == QMetaType::Bool)
        return v.toBool();
    bool out = fallback;
    if (!validateBool(v.toString(), bindingFor(attribute)->attributeName(), out,
                      bindingFor(attribute)->path()))
        return fallback;
    return out;
}

/// @brief 求值绑定为 QString：非字符串型报告诊断并返回 fallback。
QString Element::resolveString(const QString& attribute, const LayoutContext& ctx, const QString& fallback) const
{
    QVariant v = boundValue(attribute, ctx);
    if (!v.isValid())
        return fallback;
    const Binding* b = bindingFor(attribute);
    if (v.userType() != QMetaType::QString) {
        Diagnostics::reportRuntime(ErrorCode::BoundValueTypeError, b->path(),
                                   QStringLiteral("Attribute %1 expects a string, got type %2")
                                       .arg(b->attributeName(), QLatin1String(v.typeName())));
        return fallback;
    }
    return v.toString();
}

/// @brief 命名空间感知的字面量属性存在性判定（语义见头文件注释）。
bool Element::hasLiteralAttribute(const QDomElement& xml, const QString& name)
{
    QDomNamedNodeMap attrs = xml.attributes();
    for (int i = 0; i < attrs.size(); ++i) {
        QDomAttr attr = attrs.item(i).toAttr();
        if (attr.namespaceURI().isEmpty() && attr.name() == name)
            return true;
    }
    return false;
}

/// @brief 命名空间感知的字面量属性取值（语义见头文件注释）。
QString Element::literalAttribute(const QDomElement& xml, const QString& name, const QString& def)
{
    QDomNamedNodeMap attrs = xml.attributes();
    for (int i = 0; i < attrs.size(); ++i) {
        QDomAttr attr = attrs.item(i).toAttr();
        if (attr.namespaceURI().isEmpty() && attr.name() == name)
            return attr.value();
    }
    return def;
}

} // namespace BroadItem
