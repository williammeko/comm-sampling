#pragma once

#include <QByteArray>
#include <QMutex>
#include <vector>

namespace comm {

// Thread-safe, bounded FIFO of raw bytes. When the capacity is exceeded the
// oldest bytes are dropped first. Producers (I/O channels) append, the
// consumer (matcher pipeline) takes.
class ByteCache
{
public:
    explicit ByteCache(int capacityBytes = 10000);

    void setCapacity(int bytes);
    int capacity() const;
    int size() const;

    void append(const char* data, int len);
    void append(const QByteArray& data);

    // Removes and returns up to `maxBytes` bytes (all if maxBytes < 0).
    QByteArray take(int maxBytes = -1);

    void clear();

private:
    mutable QMutex m_mutex;
    std::vector<char> m_buf;
    int m_capacity = 10000;
    int m_head = 0;  // index of the oldest valid byte
    int m_count = 0; // number of valid bytes
};

} // namespace comm
