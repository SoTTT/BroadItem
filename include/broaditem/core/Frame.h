#pragma once

#include <QPainter>
#include <QImage>
#include <QSizeF>
#include <QRectF>
#include <QVariant>
#include <QString>
#include <memory>

#include <broaditem/element/Element.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/PropertyContext.h>

namespace BroadItem {

/// @brief 绘制无关的布局帧，持有元素树及其上下文。
///
/// 提供与 BroadItem 相同的高层操作（加载、属性绑定、布局、绘制），
/// 但解除了对 QGraphicsObject 的依赖，可脱离 QGraphicsScene 独立使用。
class Frame {
public:
    /// @brief 从 XML 文件路径构造 Frame。
    /// @param xmlPath XML 布局文件路径。
    /// @param ctx     属性上下文，为 nullptr 时自动创建默认上下文。
    static Frame fromFile(const QString& xmlPath,
                          std::shared_ptr<PropertyContext> ctx = nullptr);

    /// @brief 从已注册的布局 ID 构造 Frame。
    /// @param layoutId 布局注册 ID。
    /// @param ctx      属性上下文，为 nullptr 时自动创建默认上下文。
    static Frame fromRegistry(int layoutId,
                              std::shared_ptr<PropertyContext> ctx = nullptr);

    /// @brief 设置动态属性值，如果该属性被元素绑定则触发重新布局。
    /// @param name  属性名。
    /// @param value 属性值。
    void setDynamicProperty(const QString& name, const QVariant& value);

    /// @brief 获取动态属性值。
    /// @param name 属性名。
    /// @return 属性值，不存在时返回无效 QVariant。
    QVariant dynamicProperty(const QString& name) const;

    /// @brief 检查动态属性是否存在。
    /// @param name 属性名。
    /// @return 存在返回 true，否则 false。
    bool hasDynamicProperty(const QString& name) const;

    /// @brief 替换属性上下文并触发重新布局。
    /// @param ctx 新的属性上下文。
    void setPropertyContext(std::shared_ptr<PropertyContext> ctx);

    /// @brief 执行完整的测量/布局周期，返回计算后的尺寸。
    /// @param availableWidth  可用宽度，-1 表示无限制。
    /// @param availableHeight 可用高度，-1 表示无限制。
    /// @return 布局后的帧尺寸。
    QSizeF performLayout(double availableWidth = -1, double availableHeight = -1);

    /// @brief 使用给定的 painter 渲染帧。
    /// @param painter 目标 QPainter，必须已初始化。
    void paint(QPainter* painter) const;

    /// @brief 返回帧的尺寸。
    /// @return 最近一次 performLayout 计算出的尺寸。
    QSizeF size() const;

    /// @brief 将帧渲染为 QImage。
    /// @param dpr 设备像素比，默认 1.0。
    /// @return 渲染后的图像。
    QImage toImage(double dpr = 1.0) const;

    /// @brief 获取当前属性上下文。
    /// @return 属性上下文指针，可能为 nullptr。
    std::shared_ptr<PropertyContext> propertyContext() const;

    /// @brief 检查指定属性是否被元素树中的元素绑定。
    /// @param name 属性名。
    /// @return 绑定返回 true，否则 false。
    bool bindsProperty(const QString& name) const;

private:
    ElementPtr m_rootElement;                          ///< 解析后的布局树根元素。
    std::shared_ptr<PropertyContext> m_propertyContext; ///< 数据绑定的属性上下文。
    mutable LayoutContext m_context;                    ///< 包装属性上下文的布局上下文（mutable 以支持 const paint 中设置 ctx）。
    QRectF m_boundingRect;                              ///< 缓存的包围矩形。

    /// @brief 设置属性上下文（如果未提供则创建默认的 MapPropertyContext）。
    void setupPropertyContext();
};

} // namespace BroadItem
