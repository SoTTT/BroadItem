#pragma once

#include <QString>
#include <memory>
#include "LayoutContext.h"

class QDomElement;

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

class XmlLayoutParser {
public:
    static ElementPtr parseFile(const QString& filePath);
    static ElementPtr parseString(const QString& xmlContent);

private:
    static ElementPtr parseNode(const QDomElement& xml);
    static ElementPtr createElement(const QString& tagName);
    static void applyPseudoAttributes(ElementPtr element, const QDomElement& xml);
};

} // namespace BroadItem
