#pragma once

#include <QPainter>
#include <QImage>
#include <QSizeF>
#include <QRectF>
#include <QVariant>
#include <QString>
#include <functional>
#include <memory>

#include <broaditem/element/Element.h>
#include <broaditem/core/Node.h>
#include <broaditem/context/LayoutContext.h>
#include <broaditem/context/PropertyContext.h>

class QObject;

namespace BroadItem {

/// @brief 布局更新策略。
enum class UpdatePolicy {
    Synchronous,  ///< 绑定属性变更立即重布局。
    Coalesced     ///< 同一事件循环回合内多次变更合并为一次重布局（默认）。
};

/// @brief 绘制无关的布局帧，持有模板元素树、实例节点树及其上下文。
///
/// 提供与 BroadItem 相同的高层操作（加载、属性绑定、布局、绘制），
/// 但解除了对 QGraphicsObject 的依赖，可脱离 QGraphicsScene 独立使用。
class Frame {
public:
    /// @brief 从 XML 文件路径构造 Frame。
    /// @param xmlPath XML 布局文件路径。
    /// @param ctx     属性上下文，为 nullptr 时自动创建默认上下文。
    /// @return 新构造的 Frame（堆分配，回调捕获 this 安全）。
    static std::unique_ptr<Frame> fromFile(const QString& xmlPath,
                                           std::shared_ptr<PropertyContext> ctx = nullptr);

    /// @brief 从已注册的布局 ID 构造 Frame。
    /// @param layoutId 布局注册 ID。
    /// @param ctx      属性上下文，为 nullptr 时自动创建默认上下文。
    /// @return 新构造的 Frame（堆分配，回调捕获 this 安全）。
    static std::unique_ptr<Frame> fromRegistry(int layoutId,
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

    /// @brief 标记实例树失效：下次 performLayout 重新物化。
    void invalidate() { m_dirty = true; }

    /// @brief 设置布局更新策略（默认 Coalesced）。
    /// @param policy 新策略。
    void setUpdatePolicy(UpdatePolicy policy) { m_updatePolicy = policy; }

    /// @brief 返回当前布局更新策略。
    /// @return 当前生效的更新策略。
    UpdatePolicy updatePolicy() const { return m_updatePolicy; }

    /// @brief 立即执行待定的合并更新；无待定时无操作（幂等）。
    void flush();

    /// @brief 是否有待定的合并更新。
    /// @return 存在已排入事件循环但尚未执行的合并更新时返回 true。
    bool hasPendingUpdate() const { return m_updateScheduled; }

    /// @brief 替换重布局动作（默认为调用 performLayout()）。
    ///
    /// 供持有方（如 BroadItem）注入 prepareGeometryChange()/update() 等外围
    /// 通知；自定义动作必须自行调用 performLayout()。
    ///
    /// @param action 新的重布局动作。
    void setRelayoutAction(std::function<void()> action)
    {
        m_relayoutAction = std::move(action);
    }

    /// @brief 执行完整的物化（必要时）/测量/布局周期，返回计算后的尺寸。
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

    /// @brief 检查指定属性是否被模板元素树中的元素绑定。
    /// @param name 属性名。
    /// @return 绑定返回 true，否则 false。
    bool bindsProperty(const QString& name) const;

private:
    Frame() = default;

    ElementPtr m_rootTemplate;                          ///< 共享、不可变的模板树（Registry 安全）。
    std::unique_ptr<Node> m_rootNode;                   ///< 每实例节点树（物化产物）。
    bool m_dirty = true;                                ///< 数据变更后为 true，下次 performLayout 重新物化。
    std::shared_ptr<PropertyContext> m_propertyContext; ///< 数据绑定的属性上下文。
    mutable LayoutContext m_context;                    ///< 包装属性上下文的布局上下文（mutable 以支持 const paint 中设置 ctx）。
    QRectF m_boundingRect;                              ///< 缓存的包围矩形。
    UpdatePolicy m_updatePolicy = UpdatePolicy::Coalesced;          ///< 布局更新策略。
    bool m_updateScheduled = false;                                 ///< 合并更新已排入事件循环（去重守卫）。
    std::function<void()> m_relayoutAction = [this] { performLayout(); }; ///< 重布局动作，可被持有方替换。
    std::unique_ptr<QObject> m_timerContext;                        ///< singleShot 的 context，Frame 销毁时待定任务自动取消。

    /// @brief 连接属性上下文的变更回调（默认 MapPropertyContext 的创建在 fromFile/fromRegistry）。
    void setupPropertyContext();

    /// @brief 标脏并按更新策略调度重布局（同步立即执行 / 合并排入事件循环）。
    void requestUpdate();

public:
    /// @brief 析构函数（unique_ptr 持有不完整类型，需外联定义）。
    ~Frame();
};

} // namespace BroadItem
