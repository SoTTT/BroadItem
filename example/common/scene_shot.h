#pragma once

/// @file scene_shot.h
/// @brief 示例共享辅助：场景离屏渲染 PNG 与 --smoke / --shot 命令行参数处理
///        （原在 multi_instance 与 status_panel 中逐字重复）。

#include <QCoreApplication>
#include <QDebug>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <QTimer>

/// @brief 将整个场景按场景矩形离屏渲染为 PNG 并保存。
/// @param scene 目标场景。
/// @param filePath PNG 输出路径。
/// @return 保存成功返回 true，否则 false。
inline bool renderSceneToPng(QGraphicsScene& scene, const QString& filePath)
{
    QImage image(scene.sceneRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();
    return image.save(filePath);
}

/// @brief 处理 --smoke / --shot <path> 命令行参数。
///
/// --smoke：约 3 秒后自动退出（退出码 0），用于 offscreen 冒烟验证；
/// --shot <path>：约 2.6 秒后将场景离屏渲染为 PNG 落盘并退出，
/// 保存失败以退出码 1 结束。两者可独立使用。
///
/// @param app 应用对象（定时器上下文与退出目标）。
/// @param scene 截图目标场景。
inline void handleSmokeShotArgs(QCoreApplication& app, QGraphicsScene& scene)
{
    const QStringList args = QCoreApplication::arguments();
    if (args.contains(QStringLiteral("--smoke")))
        QTimer::singleShot(3000, &app, &QCoreApplication::quit);

    const int shotIndex = args.indexOf(QStringLiteral("--shot"));
    if (shotIndex != -1 && shotIndex + 1 < args.size()) {
        const QString shotPath = args.at(shotIndex + 1);
        QTimer::singleShot(2600, &app, [&scene, shotPath]() {
            if (!renderSceneToPng(scene, shotPath)) {
                qWarning() << "PNG 保存失败:" << shotPath;
                QCoreApplication::exit(1);
                return;
            }
            QCoreApplication::quit();
        });
    }
}
