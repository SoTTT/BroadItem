#pragma once

#include <QString>
#include <QMap>
#include <memory>

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

/// @brief 按整数 ID 索引的命名/可复用布局定义的单例注册表。
class LayoutRegistry {
public:
    /// @brief 获取全局单例实例。
    static LayoutRegistry& instance();

    /// @brief 从目录加载所有布局 XML 文件并注册。
    int loadLayoutsFromDirectory(const QString& dirPath);
    /// @brief 使用 ID 注册布局元素树。
    void registerLayout(int id, const ElementPtr& root);
    /// @brief 通过 ID 获取已注册的布局。
    ElementPtr getLayout(int id) const;

private:
    LayoutRegistry() = default;
    QMap<int, ElementPtr> m_layouts;  ///< Map of registered layout ID to element tree root.
    int m_nextId = 1;                  ///< Auto-incrementing ID counter for layouts without explicit IDs.
};

/// @brief 全局辅助函数，从目录加载布局。
int loadLayoutsFromDirectory(const QString& dirPath);

} // namespace BroadItem
