#pragma once

#include <QString>
#include <QMap>
#include <QMutex>
#include <memory>
#include <broaditem/element/Element.h>

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

/// @brief 按整数 ID 索引的命名/可复用布局定义的单例注册表。
///
/// 线程安全：注册/查询/目录加载经内部互斥锁保护（返回的 ConstElementPtr
/// 指向的模板树为只读共享，可安全跨线程消费）。
class LayoutRegistry {
public:
    /// @brief 获取全局单例实例。
    static LayoutRegistry& instance();

    /// @brief 从目录加载所有布局 XML 文件并注册。
    int loadLayoutsFromDirectory(const QString& dirPath);
    /// @brief 使用 ID 注册布局元素树。
    void registerLayout(int id, const ConstElementPtr& root);
    /// @brief 通过 ID 获取已注册的布局。
    ConstElementPtr getLayout(int id) const;

private:
    LayoutRegistry() = default;
    QMap<int, ConstElementPtr> m_layouts;  ///< 已注册布局 ID 到元素树根的映射。
    int m_nextId = 1;                  ///< 未显式指定 ID 的布局所用的自增 ID 计数器。
    mutable QMutex m_mutex;            ///< 保护 m_layouts 与 m_nextId 的互斥锁。
};

/// @brief 全局辅助函数，从目录加载布局。
int loadLayoutsFromDirectory(const QString& dirPath);

} // namespace BroadItem
