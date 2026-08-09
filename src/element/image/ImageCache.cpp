/// @file ImageCache.cpp
/// @brief 进程级图像缓存实现——单互斥锁保护成功/失败两张表。

#include "ImageCache.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>

namespace BroadItem {
namespace ImageCache {
namespace {

/// 缓存存储：成功表 + 失败表 + 保护二者的一把锁。
struct Store {
    QHash<QString, QPixmap> hits;   ///< 路径 → 图像（成功缓存）。
    QSet<QString> failed;           ///< 加载失败的路径集合。
    QMutex mutex;                   ///< 保护 hits/failed 的互斥锁。
};

Store& store()
{
    static Store s;
    return s;
}

} // namespace

Lookup lookup(const QString& path, QPixmap& out)
{
    QMutexLocker lock(&store().mutex);
    const auto it = store().hits.constFind(path);
    if (it != store().hits.constEnd()) {
        out = it.value();
        return Lookup::Hit;
    }
    if (store().failed.contains(path))
        return Lookup::Failed;
    return Lookup::Miss;
}

void storeHit(const QString& path, const QPixmap& pixmap)
{
    QMutexLocker lock(&store().mutex);
    store().hits.insert(path, pixmap);
}

void storeFailed(const QString& path)
{
    QMutexLocker lock(&store().mutex);
    store().failed.insert(path);
}

} // namespace ImageCache
} // namespace BroadItem
