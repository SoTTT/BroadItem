#pragma once

#include "Element.h"
#include <QDomElement>

namespace BroadItem {

class SizedElement : public Element {
public:
    virtual ~SizedElement() = default;

    void parse(const QDomElement& xml) override;

    double width() const { return m_width; }
    double height() const { return m_height; }
    bool hasWidth() const { return m_width >= 0; }
    bool hasHeight() const { return m_height >= 0; }

protected:
    double m_width = -1;   // -1 means not specified
    double m_height = -1;
};

} // namespace BroadItem
