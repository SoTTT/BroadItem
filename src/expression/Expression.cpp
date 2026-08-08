#include <broaditem/expression/Expression.h>

namespace BroadItem {

namespace {

/// @brief 标识符首字符：Unicode 字母（含中文等）或下划线。
bool isIdentStart(QChar c)
{
    return c == QLatin1Char('_') || c.isLetter();
}

/// @brief 标识符后续字符：Unicode 字母、数字或下划线。
bool isIdentContinue(QChar c)
{
    return c == QLatin1Char('_') || c.isLetterOrNumber();
}

} // namespace

/// @brief 用原始路径字符串构造表达式并解析分段。
/// @param path 原始路径字符串（如 "device.cpu"）。
Expression::Expression(const QString& path)
    : m_path(path)
{
    parse();
}

/// @brief 将路径字符串解析为类型化分段序列（语法规则见头文件类注释）。
void Expression::parse()
{
    m_valid = false;
    m_segments.clear();

    const int len = m_path.length();
    if (len == 0)
        return;

    int pos = 0;
    while (pos < len) {
        // ---- 键分段：读到 '.'、'[' 或结尾 ----
        int segEnd = pos;
        while (segEnd < len && m_path[segEnd] != QLatin1Char('.') && m_path[segEnd] != QLatin1Char('['))
            segEnd++;
        // 空键：前导 '.'、前导 '['、连续 '.'、'.[' 均在此被拦下
        if (segEnd == pos)
            return;
        const QString key = m_path.mid(pos, segEnd - pos);
        // 键分段须为 Unicode 标识符：首字符字母或 '_'，后续允许字母/数字/'_'
        //（排除数字开头与连字符/空格等数据键坏味道，真实非 ASCII 数据键存活）
        if (!isIdentStart(key[0]))
            return;
        for (int i = 1; i < key.length(); ++i) {
            if (!isIdentContinue(key[i]))
                return;
        }
        m_segments.push_back(PathSegment::makeKey(key));
        pos = segEnd;

        // ---- 索引分段：紧跟的连续 [n] ----
        while (pos < len && m_path[pos] == QLatin1Char('[')) {
            const int closePos = m_path.indexOf(QLatin1Char(']'), pos);
            if (closePos < 0)
                return;  // 开括号未闭合
            const QString indexStr = m_path.mid(pos + 1, closePos - pos - 1);
            bool ok = false;
            const int index = indexStr.toInt(&ok);
            if (!ok || index < 0)
                return;  // 非整数或负索引
            m_segments.push_back(PathSegment::makeIndex(index));
            pos = closePos + 1;
            // ']' 后紧跟下一个 '['：继续索引循环（a[0][1]）
        }

        if (pos >= len)
            break;
        // ']' 或键之后只允许 '.' 继续；a[0]b 之类的裸连接在此被拦下
        if (m_path[pos] != QLatin1Char('.'))
            return;
        pos++;
        // 结尾 '.'（a.、a[0].）：下一轮循环会因空键返回，此处提前判定
        if (pos >= len)
            return;
    }

    m_valid = true;
}

/// @brief 路径前缀匹配（唯一实现）：相等或以 "propName." / "propName[" 开头。
bool Expression::pathMatches(const QString& bindPath, const QString& propName)
{
    return bindPath == propName
        || bindPath.startsWith(propName + QLatin1Char('.'))
        || bindPath.startsWith(propName + QLatin1Char('['));
}

} // namespace BroadItem
