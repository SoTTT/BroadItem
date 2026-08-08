#include <broaditem/context/PropertyContext.h>

namespace BroadItem {

// 分段路径遍历（唯一实现）：消费 Expression 分段，逐段类型断言与存在性检查。
QVariant PropertyContext::walkSegments(QVariant current, const std::vector<PathSegment>& segs,
                                       size_t start, const QString& path)
{
    for (size_t i = start; i < segs.size(); ++i) {
        const PathSegment& seg = segs[i];
        if (seg.isIndex()) {
            // 类型检查：[n] 索引要求父级为 QMetaType::QVariantList。
            if (current.userType() != QMetaType::QVariantList) {
                Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                           QStringLiteral("cannot index into non-array type %1")
                                               .arg(QLatin1String(current.typeName())));
                return QVariant();
            }
            const QVariantList list = current.toList();
            if (seg.index() >= list.size()) {
                Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                           QStringLiteral("index %1 out of bounds, size %2")
                                               .arg(seg.index()).arg(list.size()));
                return QVariant();
            }
            current = list.at(seg.index());
        } else {
            // 类型检查：.key 导航要求父级为 QMetaType::QVariantMap。
            if (current.userType() != QMetaType::QVariantMap) {
                Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                           QStringLiteral("cannot access %1 on non-object type %2")
                                               .arg(seg.key(), QLatin1String(current.typeName())));
                return QVariant();
            }
            const QVariantMap map = current.toMap();
            if (!map.contains(seg.key())) {
                Diagnostics::reportRuntime(ErrorCode::NestedKeyMissing, path,
                                           QStringLiteral("%1 not found in object").arg(seg.key()));
                return QVariant();
            }
            current = map.value(seg.key());
        }
    }
    return current;
}

// 基于 QObject 属性的路径遍历：Expression 统一解析，首段走 QObject::property()。
QVariant PropertyContext::walkPathFromObject(const QObject* obj, const QString& path)
{
    if (!obj)
        return QVariant();

    const Expression expr(path);
    if (!expr.isValid()) {
        Diagnostics::reportRuntime(ErrorCode::PathSyntaxError, path,
                                   QStringLiteral("invalid path syntax"));
        return QVariant();
    }
    const auto& segs = expr.segments();
    // 语法保证首段必为键分段（前导 '[' 在 Expression 侧已拒绝）。

    QVariant current = obj->property(segs.front().key().toUtf8().constData());
    if (!current.isValid()) {
        Diagnostics::reportRuntime(ErrorCode::ObjectFirstKeyMissing, path,
                                   QStringLiteral("property %1 not found on object").arg(segs.front().key()));
        return QVariant();
    }

    return walkSegments(current, segs, 1, path);
}

// 分段嵌套写入：递归下沉，叶子写值后逐层回写；失败无副作用。
bool PropertyContext::setWalkSegments(QVariant& current, const std::vector<PathSegment>& segs,
                                      size_t start, const QString& path, const QVariant& value)
{
    for (size_t i = start; i < segs.size(); ++i) {
        const PathSegment& seg = segs[i];
        const bool leaf = (i + 1 == segs.size());
        if (seg.isIndex()) {
            if (current.userType() != QMetaType::QVariantList) {
                Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                           QStringLiteral("cannot index into non-array type %1")
                                               .arg(QLatin1String(current.typeName())));
                return false;
            }
            QVariantList list = current.toList();
            if (seg.index() >= list.size()) {
                Diagnostics::reportRuntime(ErrorCode::IndexOutOfBounds, path,
                                           QStringLiteral("index %1 out of bounds, size %2")
                                               .arg(seg.index()).arg(list.size()));
                return false;
            }
            if (leaf) {
                list[seg.index()] = value;
                current = list;
                return true;
            }
            QVariant child = list.at(seg.index());
            if (!setWalkSegments(child, segs, i + 1, path, value))
                return false;
            list[seg.index()] = child;
            current = list;
            return true;
        }

        if (current.userType() != QMetaType::QVariantMap) {
            Diagnostics::reportRuntime(ErrorCode::TraverseTypeMismatch, path,
                                       QStringLiteral("cannot access %1 on non-object type %2")
                                           .arg(seg.key(), QLatin1String(current.typeName())));
            return false;
        }
        QVariantMap map = current.toMap();
        if (!map.contains(seg.key())) {
            Diagnostics::reportRuntime(ErrorCode::NestedKeyMissing, path,
                                       QStringLiteral("%1 not found in object").arg(seg.key()));
            return false;
        }
        if (leaf) {
            map[seg.key()] = value;
            current = map;
            return true;
        }
        QVariant child = map.value(seg.key());
        if (!setWalkSegments(child, segs, i + 1, path, value))
            return false;
        map[seg.key()] = child;
        current = map;
        return true;
    }
    return false;
}

} // namespace BroadItem
