#pragma once

#include <broaditem/context/PropertyContext.h>
#include <broaditem/context/MapPropertyContext.h>

namespace BroadItem {

/**
 * @brief 链式属性上下文，提供 per-item 优先 + fallback 全局的解析逻辑。
 *
 * 同时持有 itemContext（MapPropertyContext，只读）和 globalContext（PropertyContext），
 * 解析时优先从 itemContext 查找（支持 asVariable 前缀剥离），未命中则 fallback 到 globalContext。
 * setProperty 直接委托到 globalContext；itemContext 为只读。
 */
class ItemPropertyContext : public PropertyContext {
public:
    /**
     * @brief 构造链式上下文。
     * @param itemContext  当前迭代项的属性上下文（可为 nullptr）。
     * @param globalContext 全局属性上下文（可为 nullptr）。
     * @param asVariable   `<for as="xxx">` 中的迭代变量名，用于前缀剥离。
     */
    ItemPropertyContext(MapPropertyContext* itemContext,
                        PropertyContext* globalContext,
                        const QString& asVariable);

    /** @copydoc PropertyContext::property */
    QVariant property(const QString& name) const override;

    /** @copydoc PropertyContext::hasProperty */
    bool hasProperty(const QString& name) const override;

    /** @copydoc PropertyContext::setProperty */
    void setProperty(const QString& name, const QVariant& value) override;

private:
    MapPropertyContext* m_itemContext;   ///< Per-item context（只读）。
    PropertyContext*    m_globalContext; ///< 全局 fallback context。
    QString             m_asVariable;    ///< `<for as="...">` 变量名。

    /** @brief 剥离 asVariable 前缀。若不以 "asVariable." 开头则原样返回。 */
    QString stripAsPrefix(const QString& name) const;
};

} // namespace BroadItem
