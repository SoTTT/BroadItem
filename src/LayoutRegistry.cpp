#include "broaditem/LayoutRegistry.h"
#include "broaditem/XmlLayoutParser.h"
#include <QDir>
#include <QDebug>

namespace BroadItem {

/// @brief 返回布局注册表的单例实例。
/// @return Reference to the global LayoutRegistry.
LayoutRegistry& LayoutRegistry::instance()
{
    static LayoutRegistry inst;
    return inst;
}

/// @brief 从目录加载所有 XML 布局文件到注册表。
/// @param dirPath Path to the directory containing .xml layout files.
/// @return Number of layouts successfully loaded.
int LayoutRegistry::loadLayoutsFromDirectory(const QString& dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        qWarning() << "Directory does not exist:" << dirPath;
        return 0;
    }

    QStringList filters;
    filters << "*.xml";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    int count = 0;
    for (const QFileInfo& info : files) {
        auto root = XmlLayoutParser::parseFile(info.absoluteFilePath());
        if (root) {
            int id = m_nextId++;
            m_layouts.insert(id, root);
            ++count;
        } else {
            qWarning() << "Failed to parse layout file, skipping:" << info.absoluteFilePath();
        }
    }
    return count;
}

/// @brief 在给定 ID 下注册布局元素树。
/// @param id The identifier to associate with the layout.
/// @param root The root element of the layout tree.
void LayoutRegistry::registerLayout(int id, const ElementPtr& root)
{
    m_layouts.insert(id, root);
}

/// @brief 按 ID 检索注册的布局。
/// @param id The layout identifier.
/// @return The root element, or nullptr if not found.
ElementPtr LayoutRegistry::getLayout(int id) const
{
    return m_layouts.value(id, nullptr);
}

/// @brief 从目录加载布局的便捷自由函数包装。
/// @param dirPath Path to the directory containing .xml layout files.
/// @return Number of layouts successfully loaded.
int loadLayoutsFromDirectory(const QString& dirPath)
{
    return LayoutRegistry::instance().loadLayoutsFromDirectory(dirPath);
}

} // namespace BroadItem
