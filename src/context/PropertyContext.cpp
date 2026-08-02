#include <broaditem/context/PropertyContext.h>

namespace BroadItem {

// 纯嵌套路径遍历引擎：从已解析的首段值出发，递进处理 .key 与 [n]。
QVariant PropertyContext::walkNested(QVariant current, const QString& path, int pos)
{
    int len = path.length();

    while (pos < len) {
        int segEnd = segmentEnd(path, pos);

        QString key = path.mid(pos, segEnd - pos);
        if (key.isEmpty()) {
            Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                       QStringLiteral("empty key in path"));
            return QVariant();
        }

        // 类型检查：.key 导航要求父级为 QMetaType::QVariantMap。
        if (current.userType() == QMetaType::QVariantMap) {
            QVariantMap map = current.toMap();
            if (!map.contains(key)) {
                Diagnostics::reportRuntime(ErrorCode::NestedKeyMissing, path,
                                           QStringLiteral("%1 not found in object").arg(key));
                return QVariant();
            }
            current = map.value(key);
        } else {
            Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                       QStringLiteral("cannot access %1 on non-object type %2")
                                           .arg(key, QLatin1String(current.typeName())));
            return QVariant();
        }

        pos = segEnd;
        pos = advanceBrackets(current, path, pos);
        if (pos < 0)
            return QVariant();

        if (pos < len) {
            if (path[pos] != '.') {
                Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                           QStringLiteral("expected '.' or end, got %1")
                                               .arg(path[pos]));
                return QVariant();
            }
            pos++;
        }
    }

    return current;
}

// 基于 QObject 属性的路径遍历：首段走 QObject::property()，余下交给 walkNested。
QVariant PropertyContext::walkPathFromObject(const QObject* obj, const QString& path)
{
    if (!obj)
        return QVariant();

    int len = path.length();
    int segEnd = segmentEnd(path, 0);

    QString firstKey = path.left(segEnd);
    QVariant current = obj->property(firstKey.toUtf8().constData());
    if (!current.isValid()) {
        Diagnostics::reportRuntime(ErrorCode::ObjectFirstKeyMissing, path,
                                   QStringLiteral("property %1 not found on object").arg(firstKey));
        return QVariant();
    }

    int pos = advanceBrackets(current, path, segEnd);
    if (pos < 0)
        return QVariant();

    if (pos < len) {
        if (path[pos] != '.') {
            Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                       QStringLiteral("expected '.' or end, got %1")
                                               .arg(path[pos]));
            return QVariant();
        }
        pos++;
    }

    return walkNested(current, path, pos);
}

// 嵌套写入辅助：沿 path[pos:] 遍历 current，在叶子位置设置 value；失败返回 false 且不修改 current。
bool PropertyContext::setWalkInto(QVariant& current, const QString& path, int pos, const QVariant& value)
{
    int len = path.length();

    while (pos < len) {
        if (path[pos] == '[') {
            int closePos = path.indexOf(']', pos);
            if (closePos < 0) {
                Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                           QStringLiteral("unmatched '['"));
                return false;
            }
            QString indexStr = path.mid(pos + 1, closePos - pos - 1);
            bool ok;
            int index = indexStr.toInt(&ok);
            if (!ok || index < 0) {
                Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                           QStringLiteral("invalid index %1").arg(indexStr));
                return false;
            }
            if (current.userType() != QMetaType::QVariantList) {
                Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                           QStringLiteral("cannot index into non-array type %1")
                                               .arg(QLatin1String(current.typeName())));
                return false;
            }
            QVariantList list = current.toList();
            if (index >= list.size()) {
                Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                           QStringLiteral("index %1 out of bounds, size %2")
                                               .arg(index).arg(list.size()));
                return false;
            }

            pos = closePos + 1;
            if (pos >= len) {
                list[index] = value;
                current = list;
                return true;
            }

            if (path[pos] != '.' && path[pos] != '[') {
                Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                           QStringLiteral("expected '.' or '[' after ']', got %1")
                                                   .arg(path[pos]));
                return false;
            }

            QVariant child = list.at(index);
            if (!setWalkInto(child, path, pos, value))
                return false;
            list[index] = child;
            current = list;
            return true;

        } else if (path[pos] == '.') {
            pos++;

            int segEnd = segmentEnd(path, pos);

            QString key = path.mid(pos, segEnd - pos);
            if (key.isEmpty()) {
                Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                           QStringLiteral("empty key in path"));
                return false;
            }

            if (current.userType() != QMetaType::QVariantMap) {
                Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                           QStringLiteral("cannot access %1 on non-object type %2")
                                               .arg(key, QLatin1String(current.typeName())));
                return false;
            }
            QVariantMap map = current.toMap();
            if (!map.contains(key)) {
                Diagnostics::reportRuntime(ErrorCode::NestedKeyMissing, path,
                                           QStringLiteral("%1 not found in object").arg(key));
                return false;
            }

            int savedSegEnd = segEnd;
            pos = savedSegEnd;
            if (pos >= len) {
                map[key] = value;
                current = map;
                return true;
            }

            QVariant child = map.value(key);
            if (!setWalkInto(child, path, pos, value))
                return false;
            map[key] = child;
            current = map;
            return true;

        } else {
            Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                       QStringLiteral("unexpected character %1")
                                           .arg(path[pos]));
            return false;
        }
    }

    return false;
}

// 消化路径中的数组索引 [n]，修改 current 并返回新位置；出错返回 -1。
int PropertyContext::advanceBrackets(QVariant& current, const QString& path, int pos)
{
    int len = path.length();

    while (pos < len && path[pos] == '[') {
        int closePos = path.indexOf(']', pos);
        if (closePos < 0) {
            Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                       QStringLiteral("unmatched '['"));
            return -1;
        }
        QString indexStr = path.mid(pos + 1, closePos - pos - 1);
        bool ok;
        int index = indexStr.toInt(&ok);
        if (!ok || index < 0) {
            Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                       QStringLiteral("invalid index %1").arg(indexStr));
            return -1;
        }
        // 类型检查：[n] 索引要求父级为 QMetaType::QVariantList。
        if (current.userType() != QMetaType::QVariantList) {
            Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                       QStringLiteral("cannot index into non-array type %1")
                                               .arg(QLatin1String(current.typeName())));
            return -1;
        }
        QVariantList list = current.toList();
        if (index >= list.size()) {
            Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                       QStringLiteral("index %1 out of bounds, size %2")
                                               .arg(index).arg(list.size()));
            return -1;
        }
        current = list.at(index);
        pos = closePos + 1;
    }

    return pos;
}

} // namespace BroadItem
