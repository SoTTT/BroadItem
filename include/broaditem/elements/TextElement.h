#pragma once

#include "../SizedElement.h"

namespace BroadItem {

class TextElement : public SizedElement {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const Rect& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    ElementPtr clone() const override;
    void interpolateValues(const QStringList& values) override;

private:
    QString m_text;
    QString m_contentLiteral;
    bool m_hasContentLiteral = false;
    QString m_propertyName;
    QFont m_font;
    QString m_vAlign = "baseline";
    QString m_hAlign = "left";
    bool m_bold = false;
    bool m_underLine = false;
    bool m_wrap = false;
    double m_maxWidth = -1;
    double m_fontSize = 12;
    QString m_fontFamily;
    QColor m_color = Qt::black;

    QString resolvedText(const LayoutContext& ctx) const;
    QSizeF computeTextSize(const QString& text, const LayoutConstraints& constraints) const;
};

} // namespace BroadItem
