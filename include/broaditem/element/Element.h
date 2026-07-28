#pragma once

#include <QPainter>
#include <QDomElement>
#include <QDomAttr>
#include <QStringList>
#include <QSet>
#include <QHash>
#include <QRectF>
#include <memory>
#include <vector>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/core/Node.h>
#include <broaditem/expression/Binding.h>

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
    /// 控制元素（for、if）覆盖此方法，展开为 0..N 个节点。
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
    /// 基类实现遍历通用绑定表 m_bindings（见 parseBindings）。
    virtual bool bindsProperty(const QString& name) const;

    /// @brief 解析 XML 元素上的通用绑定属性（b:attr 形式）存入 m_bindings。
    ///
    /// 遍历 xml.attributes()，命中绑定命名空间的属性取局部名（命名空间处理
    /// 开启时 QDomAttr::name() 返回不带前缀的局部名）。保留名 of/prop/as/content
    /// 与控制元素语义耦合，跳过；局部名不在 supportedAttributes() 中报告
    /// BI-P-012 并跳过；与字面量同名属性互斥（BI-P-014，忽略该绑定）。
    /// @param xml The DOM element whose binding attributes to parse.
    void parseBindings(const QDomElement& xml);

    /// @brief 按局部属性名查找通用绑定。
    /// @param attribute 局部属性名（如 "color"，不带 "b:" 前缀）。
    /// @return 命中的 Binding 指针；未命中返回 nullptr。
    const Binding* bindingFor(const QString& attribute) const;

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

    /// @brief 运行时诊断去重标记：key 首次插入返回 true，已存在返回 false。
    ///
    /// 仅模板根元素的集合被 Diagnostics::RuntimeScope 使用
    /// （仿 ImageElement::m_failedPaths 的模板级先例）。
    /// @param key 去重键（错误码|绑定路径）。
    /// @return 首次报告返回 true。
    bool markDiagnosticReported(const QString& key) const
    {
        if (m_reportedDiagnostics.contains(key))
            return false;
        m_reportedDiagnostics.insert(key);
        return true;
    }

    /// @brief 验证属性值是否为 double，有效返回 true。
    ///
    /// 校验失败时：runtimePath 为空按解析期字面量错误（BI-P-015）报告；
    /// 非空按运行时绑定值错误（BI-R-011）报告，runtimePath 为绑定路径。
    static bool validateDouble(const QString& value, const QString& attrName, double& out,
                               const QString& runtimePath = QString());
    /// @brief 验证属性值是否为 int，有效返回 true。阶段语义同 validateDouble。
    static bool validateInt(const QString& value, const QString& attrName, int& out,
                            const QString& runtimePath = QString());
    /// @brief 验证属性值是否为 bool，有效返回 true。阶段语义同 validateDouble。
    static bool validateBool(const QString& value, const QString& attrName, bool& out,
                             const QString& runtimePath = QString());

protected:
    /// @brief 返回本元素实际有求值路径的通用绑定属性集合。
    ///
    /// 与 supportedAttributes()（声明"认识哪些属性"）区分：本集合声明"哪些
    /// 绑定属性会被真正求值"。parseBindings 对不在本集合内的绑定发
    /// 「已注册未解析」告警但仍照常注册（向前兼容：求值路径后续版本补齐）。
    /// 基类返回空集；各级元素按 static 引用 + 集合并集方式覆盖
    /// （同 supportedAttributes 的 static-union 写法，返回临时 QSet 会悬垂）。
    /// @return 有求值路径的绑定属性名集合的静态引用。
    virtual const QSet<QString>& resolvedAttributes() const;

    /// @brief 从字符串解析 double，失败时返回默认值。
    static double parseDouble(const QString& value, double defaultVal = 0);
    /// @brief 从字符串解析 QColor（名称或十六进制）。
    static QColor parseColor(const QString& value);
    /// @brief 从字符串解析布尔值。
    static bool parseBool(const QString& value);

    /// @brief 命名空间感知的字面量属性存在性判定。
    ///
    /// QDom 在命名空间模式下 hasAttribute/attribute 按局部名匹配（会命中
    /// b:xxx 绑定属性自身），hasAttributeNS(QString(), ...) 又匹配不到无命名
    /// 空间字面量——parseBindings 已实证该陷阱。字面量读取一律走本辅助：
    /// 遍历属性，命中条件 namespaceURI().isEmpty() && name() == local。
    /// @param xml The DOM element to inspect.
    /// @param name 字面量属性名（如 "color"）。
    /// @return 存在无命名空间的同名字面量属性时返回 true。
    static bool hasLiteralAttribute(const QDomElement& xml, const QString& name);

    /// @brief 命名空间感知的字面量属性取值（判定语义同 hasLiteralAttribute）。
    /// @param xml The DOM element to inspect.
    /// @param name 字面量属性名。
    /// @param def 属性不存在时的默认返回值。
    /// @return 字面量属性值；不存在时返回 def。
    static QString literalAttribute(const QDomElement& xml, const QString& name,
                                    const QString& def = QString());

    /// @brief 求值通用绑定：返回绑定路径在上下文中的属性值。
    /// 无绑定、绑定无效或上下文无此属性时返回无效 QVariant（静默回退）。
    /// @param attribute 局部属性名（如 "color"）。
    /// @param ctx 布局上下文。
    /// @return 绑定值；不可解析时为无效 QVariant。
    QVariant boundValue(const QString& attribute, const LayoutContext& ctx) const;

    /// @brief 求值绑定为 double，失败返回 fallback。
    double resolveDouble(const QString& attribute, const LayoutContext& ctx, double fallback) const;
    /// @brief 求值绑定为 QColor，失败返回 fallback。
    QColor resolveColor(const QString& attribute, const LayoutContext& ctx, const QColor& fallback) const;
    /// @brief 求值绑定为 bool，失败返回 fallback。
    bool resolveBool(const QString& attribute, const LayoutContext& ctx, bool fallback) const;
    /// @brief 求值绑定为 QString，失败返回 fallback。
    QString resolveString(const QString& attribute, const LayoutContext& ctx, const QString& fallback) const;

    /// @brief 通用绑定表：局部属性名 → Binding。
    /// Binding 无默认构造函数，只允许 insert/constFind 访问。
    QHash<QString, Binding> m_bindings;

private:
    /// 运行时诊断去重集合（模板级，mutable 因报告发生在 const 管线中）。
    mutable QSet<QString> m_reportedDiagnostics;
};

} // namespace BroadItem
