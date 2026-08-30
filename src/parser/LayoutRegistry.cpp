#include <broaditem/parser/LayoutRegistry.h>
#include <broaditem/parser/XmlLayoutParser.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <QDir>
#include <QMutexLocker>

namespace BroadItem {

/// @brief 返回布局注册表的单例实例。
/// @return 全局 LayoutRegistry 的引用。
LayoutRegistry& LayoutRegistry::instance()
{
    static LayoutRegistry inst;
    return inst;
}

/// @brief 从目录加载所有 XML 布局文件到注册表。
///
/// 目录不存在报 BI-P-020（按空目录处理）；单文件解析失败报 BI-P-021 并
/// 跳过该文件（其内部错误已由 parse 会话各自报告）。
///
/// @param dirPath 包含 .xml 布局文件的目录路径。
/// @return 成功加载的布局数量。
int LayoutRegistry::loadLayoutsFromDirectory(const QString& dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        Diagnostics::reportParse(ErrorCode::RegistryDirMissing,
                                 QStringLiteral("directory does not exist: %1").arg(dirPath), dirPath);
        return 0;
    }

    QStringList filters;
    filters << QStringLiteral("*.xml");
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    int count = 0;
    for (const QFileInfo& info : files) {
        auto root = XmlLayoutParser::parseFile(info.absoluteFilePath());
        if (root) {
            // 锁仅护 ID 分配与插入；解析（慢、且自身会报诊断）在锁外进行
            int id;
            bool duplicate;
            {
                QMutexLocker lock(&m_mutex);
                id = m_nextId++;
                duplicate = m_layouts.contains(id);
                m_layouts.insert(id, root);
            }
            // 重复 ID：后写者覆盖（last-wins），诊断在锁外报告（BI-P-025）
            if (duplicate) {
                Diagnostics::reportParse(ErrorCode::RegistryDuplicateId,
                                         QStringLiteral("layout id %1 already registered, overwriting (file: %2)")
                                             .arg(id).arg(info.absoluteFilePath()),
                                         info.absoluteFilePath());
            }
            ++count;
        } else {
            Diagnostics::reportParse(ErrorCode::RegistryFileSkipped,
                                     QStringLiteral("failed to parse layout file, skipping"),
                                     info.absoluteFilePath());
        }
    }
    return count;
}

/// @brief 在给定 ID 下注册布局元素树。
///
/// ID 已存在时后写者覆盖（last-wins）并报 BI-P-025——覆盖是合法用法
/// （如热替换），但不再静默。
/// @param id 与布局关联的标识符。
/// @param root 布局树的根元素。
void LayoutRegistry::registerLayout(int id, const ConstElementPtr& root)
{
    bool duplicate;
    {
        QMutexLocker lock(&m_mutex);
        duplicate = m_layouts.contains(id);
        m_layouts.insert(id, root);
    }
    if (duplicate) {
        Diagnostics::reportParse(ErrorCode::RegistryDuplicateId,
                                 QStringLiteral("layout id %1 already registered, overwriting").arg(id));
    }
}

/// @brief 按 ID 检索注册的布局。
/// @param id 布局标识符。
/// @return 根元素；未找到时返回 nullptr。
ConstElementPtr LayoutRegistry::getLayout(int id) const
{
    QMutexLocker lock(&m_mutex);
    return m_layouts.value(id, nullptr);
}

/// @brief 从目录加载布局的便捷自由函数包装。
/// @param dirPath 包含 .xml 布局文件的目录路径。
/// @return 成功加载的布局数量。
int loadLayoutsFromDirectory(const QString& dirPath)
{
    return LayoutRegistry::instance().loadLayoutsFromDirectory(dirPath);
}

} // namespace BroadItem
