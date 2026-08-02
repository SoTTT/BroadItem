#include <broaditem/expression/Expression.h>

namespace BroadItem {

/// @brief 用原始路径字符串构造表达式并解析分段。
/// @param path 原始路径字符串（如 "device.cpu"）。
Expression::Expression(const QString& path)
    : m_path(path)
{
    parse();
}

/// @brief 将路径字符串解析为分段向量。
/// 按 '.' 分割为键名分段，按 '[' 和 ']' 分割为数组索引分段。
void Expression::parse()
{
    m_valid = true;
    m_segments.clear();

    if (m_path.isEmpty()) {
        m_valid = false;
        return;
    }

    int len = m_path.length();
    int pos = 0;
    int segStart = 0;

    while (pos < len) {
        QChar ch = m_path[pos];

        if (ch == QLatin1Char('.')) {
            // 点号前为空分段：前导点或连续两点
            if (pos == segStart) {
                m_valid = false;
                m_segments.clear();
                return;
            }
            m_segments.push_back(m_path.mid(segStart, pos - segStart));
            pos++;
            segStart = pos;
        } else if (ch == QLatin1Char('[')) {
            if (pos > segStart) {
                // 先压入方括号前的键名分段
                m_segments.push_back(m_path.mid(segStart, pos - segStart));
            } else if (pos == 0 || (pos > 0 && m_path[pos - 1] == QLatin1Char('.'))) {
                // 前导方括号或点号紧接方括号：键名为空
                m_valid = false;
                m_segments.clear();
                return;
            }
            // 其余情形：']' 后紧跟的连续方括号——合法，无键名可压入

            int closePos = m_path.indexOf(QLatin1Char(']'), pos);
            if (closePos < 0) {
                // 开括号未闭合
                m_valid = false;
                m_segments.clear();
                return;
            }

            QString indexStr = m_path.mid(pos + 1, closePos - pos - 1);
            bool ok = false;
            int index = indexStr.toInt(&ok);
            if (!ok || index < 0) {
                // 非整数或负索引
                m_valid = false;
                m_segments.clear();
                return;
            }

            m_segments.push_back(indexStr);
            pos = closePos + 1;

            // 跳过紧随闭括号的一个点号
            if (pos < len && m_path[pos] == QLatin1Char('.')) {
                pos++;
            }
            segStart = pos;
        } else {
            pos++;
        }
    }

    // 处理末尾累积的分段（如有）
    if (pos > segStart) {
        m_segments.push_back(m_path.mid(segStart, pos - segStart));
    } else if (!m_path.endsWith(QLatin1Char(']'))) {
        // 末尾空分段（如结尾点号）；路径以闭括号结尾时除外
        m_valid = false;
        m_segments.clear();
    }
}

} // namespace BroadItem
