#pragma once

/// @file ImageCache.h
/// @brief 进程级图像缓存（路径键）——<image> 部件的加载结果共享层。
///
/// 键为文件路径（同路径同图，与模板无必然绑定），因此缓存从模板 mutable
/// 成员（m_pixmapCache/m_failedPaths）收编为进程级单例：模板回到完全只读，
/// 缓存跨模板共享，多线程经互斥锁安全。缓存永不淘汰，生命周期随进程。
/// 本头为 src/element/image 内部实现细节，不对外导出。

#include <QPixmap>
#include <QString>

namespace BroadItem {
namespace ImageCache {

/// @brief 缓存查询结果。
enum class Lookup {
    Hit,    ///< 命中成功缓存，out 写入图像。
    Failed, ///< 命中失败缓存（该路径此前加载失败，跳过重复加载，静默空白）。
    Miss    ///< 未命中，调用方应尝试加载并回写结果。
};

/// @brief 查询路径的缓存状态。
/// @param path 图像路径（Qt 资源 :/ 或文件系统路径）。
/// @param out  [out] Hit 时写入缓存的图像。
/// @return 查询结果。
Lookup lookup(const QString& path, QPixmap& out);

/// @brief 写入成功缓存。
/// @param path   图像路径。
/// @param pixmap 加载成功的图像。
void storeHit(const QString& path, const QPixmap& pixmap);

/// @brief 写入失败缓存（后续同路径查询直接返回 Failed）。
/// @param path 图像路径。
void storeFailed(const QString& path);

} // namespace ImageCache
} // namespace BroadItem
