#include <QApplication>
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>

#include <broaditem/expression/Expression.h>

/// @brief 路径语法交互验证工具。
///
/// 输入任意路径字符串，实时显示 Expression 的语法校验结果与类型化分段
/// （键 / 索引）。用于人工验证路径解析器的健壮性。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle(QStringLiteral("路径语法验证（Expression / PathSegment）"));
    auto* layout = new QVBoxLayout(&window);

    auto* input = new QLineEdit;
    input->setPlaceholderText(QStringLiteral("输入路径，如 device.cpu、items[0].name、a[0][1].c"));
    layout->addWidget(input);

    auto* verdict = new QLabel;
    layout->addWidget(verdict);

    auto* table = new QTableWidget(0, 3);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("#"), QStringLiteral("类型"), QStringLiteral("内容")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table);

    // 每次输入变化都重新解析并刷新校验结果与分段表
    auto refresh = [=]() {
        const BroadItem::Expression expr(input->text());
        table->setRowCount(0);

        if (input->text().isEmpty()) {
            verdict->setText(QStringLiteral("（空输入）"));
            verdict->setStyleSheet(QString());
            return;
        }
        if (!expr.isValid()) {
            verdict->setText(QStringLiteral("✗ 非法路径（语法规则见 doc/路径语法.md §1）"));
            verdict->setStyleSheet(QStringLiteral("color: red"));
            return;
        }

        verdict->setText(QStringLiteral("✓ 合法，共 %1 个分段")
                             .arg(static_cast<int>(expr.segments().size())));
        verdict->setStyleSheet(QStringLiteral("color: green"));

        int row = 0;
        for (const auto& seg : expr.segments()) {
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(QString::number(row)));
            if (seg.isIndex()) {
                table->setItem(row, 1, new QTableWidgetItem(QStringLiteral("索引")));
                table->setItem(row, 2, new QTableWidgetItem(QString::number(seg.index())));
            } else {
                table->setItem(row, 1, new QTableWidgetItem(QStringLiteral("键")));
                table->setItem(row, 2, new QTableWidgetItem(seg.key()));
            }
            ++row;
        }
    };
    QObject::connect(input, &QLineEdit::textChanged, refresh);
    refresh();

    window.resize(440, 380);
    window.show();
    return app.exec();
}
