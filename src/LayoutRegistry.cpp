#include "broaditem/LayoutRegistry.h"
#include "broaditem/XmlLayoutParser.h"
#include <QDir>
#include <QDebug>

namespace BroadItem {

LayoutRegistry& LayoutRegistry::instance()
{
    static LayoutRegistry inst;
    return inst;
}

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

void LayoutRegistry::registerLayout(int id, const ElementPtr& root)
{
    m_layouts.insert(id, root);
}

ElementPtr LayoutRegistry::getLayout(int id) const
{
    return m_layouts.value(id, nullptr);
}

int loadLayoutsFromDirectory(const QString& dirPath)
{
    return LayoutRegistry::instance().loadLayoutsFromDirectory(dirPath);
}

} // namespace BroadItem
