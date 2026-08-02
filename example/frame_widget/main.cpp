#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <memory>
#include <broaditem/core/Frame.h>

/// @brief 持有 BroadItem::Frame 并在 paintEvent/resizeEvent 中直接渲染的 QWidget。
class FrameWidget : public QWidget {
public:
    explicit FrameWidget(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_frame(BroadItem::Frame::fromFile("frame_widget.xml"))
    {
        setWindowTitle("Frame Widget Example");
        // 本示例以显式约束 performLayout(width(), height()) 驱动布局；
        // 合并任务会以默认约束 (-1,-1) 重布局并覆盖约束结果，故保持同步策略。
        m_frame->setUpdatePolicy(BroadItem::UpdatePolicy::Synchronous);
        m_frame->setDynamicProperty("title", QStringLiteral("设备状态"));
        m_frame->setDynamicProperty("status", QStringLiteral("运行中"));
        m_frame->setDynamicProperty("ip", QStringLiteral("192.168.1.100"));
        resize(640, 480);
        m_frame->performLayout(width(), height());
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        m_frame->paint(&painter);
    }

    void resizeEvent(QResizeEvent*) override {
        m_frame->performLayout(width(), height());
        update();
    }

private:
    std::unique_ptr<BroadItem::Frame> m_frame;
};

/// @brief 入口点。创建 FrameWidget 并进入事件循环。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    FrameWidget widget;
    widget.show();
    return QApplication::exec();
}
