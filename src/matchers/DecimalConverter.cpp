#include "DecimalConverter.h"

#include <QtEndian>

#include <algorithm>

namespace comm {
namespace DecimalConverter {

QVariant convert(const QByteArray& bytes, NumberType type, ByteOrder order)
{
    if (bytes.isEmpty())
        return QVariant();

    if (type == NumberType::Ieee754) {
        if (bytes.size() >= 8) {
            const QByteArray b = bytes.left(8);
            const double d = (order == ByteOrder::BigEndian)
                ? qFromBigEndian<double>(b.constData())
                : qFromLittleEndian<double>(b.constData());
            return QVariant::fromValue(d);
        }

        if (bytes.size() >= 4) {
            const QByteArray b = bytes.left(4);
            const float f = (order == ByteOrder::BigEndian)
                ? qFromBigEndian<float>(b.constData())
                : qFromLittleEndian<float>(b.constData());
            return QVariant::fromValue(static_cast<double>(f));
        }

        return QVariant();
    }

    // UInt / Int: build the unsigned value of up to 8 bytes (MSB-first).
    QByteArray b = bytes.left(8);
    if (order == ByteOrder::LittleEndian)
        std::reverse(b.begin(), b.end());

    quint64 raw = 0;
    for (const char c : b)
        raw = (raw << 8) | static_cast<quint8>(static_cast<unsigned char>(c));

    if (type == NumberType::UInt)
        return QVariant::fromValue(raw);

    // Int: sign-extend from the actual byte width.
    const int bits = b.size() * 8;
    if (bits >= 64)
        return QVariant::fromValue(static_cast<qint64>(raw));

    const quint64 signMask = 1ull << (bits - 1);
    if (raw & signMask)
        return QVariant::fromValue(static_cast<qint64>(raw | ~((1ull << bits) - 1)));

    return QVariant::fromValue(static_cast<qint64>(raw));
}

} // namespace DecimalConverter
} // namespace comm
