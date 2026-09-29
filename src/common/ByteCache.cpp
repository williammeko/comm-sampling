#include "ByteCache.h"

#include <QtGlobal>

namespace comm {

ByteCache::ByteCache(int capacityBytes)
{
    setCapacity(capacityBytes);
}

void ByteCache::setCapacity(int bytes)
{
    QMutexLocker locker(&m_mutex);
    m_capacity = qMax(1, bytes);
    m_buf.assign(m_capacity, 0);
    m_head = 0;
    m_count = 0;
}

int ByteCache::capacity() const
{
    QMutexLocker locker(&m_mutex);
    return m_capacity;
}

int ByteCache::size() const
{
    QMutexLocker locker(&m_mutex);
    return m_count;
}

void ByteCache::append(const char* data, int len)
{
    if (!data || len <= 0)
        return;

    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < len; ++i) {
        if (m_count < m_capacity) {
            m_buf[(m_head + m_count) % m_capacity] = data[i];
            ++m_count;
        } else {
            // Full: overwrite the oldest byte.
            m_buf[m_head] = data[i];
            m_head = (m_head + 1) % m_capacity;
        }
    }
}

void ByteCache::append(const QByteArray& data)
{
    append(data.constData(), data.size());
}

QByteArray ByteCache::take(int maxBytes)
{
    QMutexLocker locker(&m_mutex);

    int n = m_count;
    if (maxBytes >= 0)
        n = qMin(n, maxBytes);

    QByteArray out;
    out.resize(n);
    for (int i = 0; i < n; ++i)
        out[i] = m_buf[(m_head + i) % m_capacity];

    m_head = (m_head + n) % m_capacity;
    m_count -= n;
    return out;
}

void ByteCache::clear()
{
    QMutexLocker locker(&m_mutex);
    m_head = 0;
    m_count = 0;
}

} // namespace comm
