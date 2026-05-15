#include <QGuiApplication>
#include <QDebug>
#include "broaditem/Element.h"
#include "broaditem/XmlLayoutParser.h"
#include "broaditem/MapPropertyContext.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    // ========== 1. 从字符串加载布局 ==========
    QString xml = R"(
        <root>
            <column space="8" padding="16">
                <text font-size="18" bold="true">标题</text>
                <text font-size="14" color="#666">副标题</text>
            </column>
        </root>
    )";

    auto element = BroadItem::XmlLayoutParser::parseString(xml);
    if (!element) {
        qCritical() << "Failed to parse XML";
        return 1;
    }

    qDebug() << "=== 1. 基本布局 ===";
    
    BroadItem::LayoutContext ctx;
    BroadItem::LayoutConstraints constraints;
    
    auto result = element->measure(ctx, constraints);
    qDebug() << "Measured size:" << result.intrinsicSize.width << "x" << result.intrinsicSize.height;

    // 分配布局区域
    BroadItem::Rect layoutRect;
    layoutRect.pos.x = 0;
    layoutRect.pos.y = 0;
    layoutRect.size = result.intrinsicSize;
    element->layout(ctx, layoutRect);

    // ========== 2. 使用数据绑定 ==========
    QString xmlWithBinding = R"(
        <root>
            <column space="4">
                <text :content="title" font-size="16" bold="true"/>
                <if-has :prop="subtitle">
                    <text :content="subtitle" font-size="12" color="#888"/>
                </if-has>
            </column>
        </root>
    )";

    auto element2 = BroadItem::XmlLayoutParser::parseString(xmlWithBinding);
    if (element2) {
        qDebug() << "\n=== 2. 数据绑定 ===";
        
        auto propCtx = std::make_shared<BroadItem::MapPropertyContext>();
        propCtx->setProperty("title", "Hello World");
        propCtx->setProperty("subtitle", "This is a subtitle");
        
        BroadItem::LayoutContext ctx2;
        ctx2.ctx = propCtx.get();
        
        auto result2 = element2->measure(ctx2, constraints);
        qDebug() << "With binding - size:" << result2.intrinsicSize.width << "x" << result2.intrinsicSize.height;
    }

    // ========== 3. 使用循环 ==========
    QString xmlWithLoop = R"(
        <root>
            <column space="4">
                <for :of="items">
                    <text font-size="12">- {}</text>
                </for>
            </column>
        </root>
    )";

    auto element3 = BroadItem::XmlLayoutParser::parseString(xmlWithLoop);
    if (element3) {
        qDebug() << "\n=== 3. 循环 ===";
        
        auto propCtx = std::make_shared<BroadItem::MapPropertyContext>();
        propCtx->setProperty("items", QStringList{"Item 1", "Item 2", "Item 3"});
        
        BroadItem::LayoutContext ctx3;
        ctx3.ctx = propCtx.get();
        
        auto result3 = element3->measure(ctx3, constraints);
        qDebug() << "With loop - size:" << result3.intrinsicSize.width << "x" << result3.intrinsicSize.height;
    }

    // ========== 4. 装饰器示例 ==========
    QString xmlWithDecorators = R"(
        <root>
            <text background-color="#E3F2FD" background-radius="8" 
                  padding="12" font-size="14">
                带背景和 padding 的文本
            </text>
        </root>
    )";

    auto element4 = BroadItem::XmlLayoutParser::parseString(xmlWithDecorators);
    if (element4) {
        qDebug() << "\n=== 4. 装饰器 ===";
        auto result4 = element4->measure(ctx, constraints);
        qDebug() << "With decorators - size:" << result4.intrinsicSize.width << "x" << result4.intrinsicSize.height;
    }

    qDebug() << "\nDone!";
    return 0;
}
