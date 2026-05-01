#pragma once

#include <QString>
#include <QMap>
#include <memory>

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

class LayoutRegistry {
public:
    static LayoutRegistry& instance();

    int loadLayoutsFromDirectory(const QString& dirPath);
    void registerLayout(int id, ElementPtr root);
    ElementPtr getLayout(int id) const;

private:
    LayoutRegistry() = default;
    QMap<int, ElementPtr> m_layouts;
    int m_nextId = 1;
};

// Global helper
int loadLayoutsFromDirectory(const QString& dirPath);

} // namespace BroadItem
