#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QDebug>
#include <broaditem/core/Frame.h>
#include <broaditem/compat/Optional.h>

/// @brief 入口点。从命令行加载 XML 布局，渲染为 QImage 并保存为 PNG。
int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QString xmlPath = argc > 1 ? QString::fromLocal8Bit(argv[1]) : "frame_image.xml";
    QString outputPath = argc > 2 ? QString::fromLocal8Bit(argv[2]) : "output.png";
    // 可选第三参数：布局可用宽度（像素），缺省为空表示内容驱动不设约束。
    BroadItem::Optional<double> availableWidth;
    if (argc > 3)
        availableWidth = QString::fromLocal8Bit(argv[3]).toDouble();

    auto frame = BroadItem::Frame::fromFile(xmlPath);
    if (!frame) {
        qCritical() << "Failed to load layout:" << xmlPath;
        return 1;
    }
    frame->performLayout(availableWidth);

    QImage image = frame->toImage(2.0);
    if (image.isNull()) {
        qCritical() << "Failed to render image";
        return 1;
    }

    if (!image.save(outputPath)) {
        qCritical() << "Failed to save image to" << outputPath;
        return 1;
    }

    qDebug() << "Saved image:" << outputPath << image.size();
    return 0;
}
