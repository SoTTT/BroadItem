#pragma once

#include <QGraphicsObject>
#include <QVariantMap>
#include <memory>

#include <broaditem/core/Frame.h>
#include <broaditem/context/PropertyContext.h>
namespace BroadItem {

/// @brief 顶层 QGraphicsItem，渲染 XML 定义的布局并支持数据绑定。
class BroadItem : public QGraphicsObject {
    Q_OBJECT
public:
    /// @brief 从 XML 文件路径构造 BroadItem。
    explicit BroadItem(const QString& xmlFilePath,
                       std::shared_ptr<PropertyContext> ctx = nullptr,
                       QGraphicsItem* parent = nullptr);
    /// @brief 从已注册的布局 ID 构造 BroadItem。
    explicit BroadItem(int layoutId,
                       std::shared_ptr<PropertyContext> ctx = nullptr,
                       QGraphicsItem* parent = nullptr);
    ~BroadItem() override;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    /// @brief 替换属性上下文并触发重新布局。
    void setPropertyContext(std::shared_ptr<PropertyContext> ctx);

    /// @brief 设置动态属性值，如果已绑定则触发重新布局。
    void setDynamicProperty(const QString& name, const QVariant& value);
    /// @brief 获取动态属性值。
    QVariant dynamicProperty(const QString& name) const;
    /// @brief 检查动态属性是否存在。
    bool hasDynamicProperty(const QString& name) const;

    /// @brief 触发完整的布局更新（测量+布局+渲染）。
    void updateLayout();

    /// @brief 通过上下文访问属性的方括号运算符语法糖。
    PropertyProxy operator[](const QString& key)
    {
        auto ctx = m_frame.propertyContext();
        return ctx ? (*ctx)[key] : PropertyProxy(nullptr, QString());
    }

private:
    Frame m_frame;                  ///< 内部布局引擎，持有元素树、上下文和布局逻辑。

    /// @brief 设置属性上下文变更回调，连接 Frame 布局与 QGraphicsItem 重绘。
    void setupPropertyContext();
};

} // namespace BroadItem
