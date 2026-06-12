#include <broaditem/expression/Expression.h>

namespace BroadItem {

Expression::Expression(const QString& path)
    : m_path(path)
{
    parse();
}

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
            // Empty segment before dot: leading dot or double dot
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
                // Push the key segment preceding the bracket
                m_segments.push_back(m_path.mid(segStart, pos - segStart));
            } else if (pos == 0 || (pos > 0 && m_path[pos - 1] == QLatin1Char('.'))) {
                // Leading bracket or dot-immediately-before-bracket: empty key
                m_valid = false;
                m_segments.clear();
                return;
            }
            // else: consecutive brackets after a ']' — valid, no key to push

            int closePos = m_path.indexOf(QLatin1Char(']'), pos);
            if (closePos < 0) {
                // Unmatched opening bracket
                m_valid = false;
                m_segments.clear();
                return;
            }

            QString indexStr = m_path.mid(pos + 1, closePos - pos - 1);
            bool ok = false;
            int index = indexStr.toInt(&ok);
            if (!ok || index < 0) {
                // Non-integer or negative index
                m_valid = false;
                m_segments.clear();
                return;
            }

            m_segments.push_back(indexStr);
            pos = closePos + 1;

            // Skip a dot immediately following a bracket close
            if (pos < len && m_path[pos] == QLatin1Char('.')) {
                pos++;
            }
            segStart = pos;
        } else {
            pos++;
        }
    }

    // Handle the final accumulated segment (if any)
    if (pos > segStart) {
        m_segments.push_back(m_path.mid(segStart, pos - segStart));
    } else if (pos == segStart && !m_path.endsWith(QLatin1Char(']'))) {
        // Trailing empty segment (e.g. trailing dot), unless path ends with a bracket
        m_valid = false;
        m_segments.clear();
    }
}

} // namespace BroadItem
