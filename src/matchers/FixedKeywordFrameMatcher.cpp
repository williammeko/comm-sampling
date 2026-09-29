#include "FixedKeywordFrameMatcher.h"

#include <QtGlobal>

namespace comm {

void FixedKeywordFrameMatcher::setKeyword(const QByteArray& keyword)
{
    m_keyword = keyword;
    if (m_keyword.size() > 10)
        m_keyword = m_keyword.left(10);
    reset();
}

void FixedKeywordFrameMatcher::setFrameLength(int length)
{
    m_frameLength = qBound(0, length, 255);
}

void FixedKeywordFrameMatcher::reset()
{
    m_buffer.clear();
}

QList<QByteArray> FixedKeywordFrameMatcher::process(const QByteArray& input)
{
    m_buffer.append(input);
    QList<QByteArray> frames;

    if (m_keyword.isEmpty()) {
        // No keyword defined: keep the tail only, to avoid unbounded growth.
        if (m_buffer.size() > 10)
            m_buffer = m_buffer.right(10);
        return frames;
    }

    const int keep = qMax(0, m_keyword.size() - 1);

    while (!m_buffer.isEmpty()) {
        const int idx = m_buffer.indexOf(m_keyword);
        if (idx < 0) {
            // Keep a tail that may contain the start of a keyword.
            if (m_buffer.size() > keep)
                m_buffer = m_buffer.right(keep);
            break;
        }

        const int available = m_buffer.size() - idx;
        QByteArray frame;

        if (available >= m_frameLength) {
            frame = m_buffer.mid(idx, m_frameLength);
            m_buffer.remove(0, idx + qMax(1, m_frameLength));
        } else {
            // Length exceeds available data: match as much as possible.
            frame = m_buffer.mid(idx, available);
            m_buffer.clear();
        }

        frames.append(frame);
    }

    return frames;
}

} // namespace comm
