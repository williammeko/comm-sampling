#include "DecimalPickerMatcher.h"

#include "matchers/DecimalConverter.h"

#include <QtGlobal>

namespace comm {

namespace {

QByteArray pickBytes(const QByteArray& frame, int start, int length)
{
    if (start < 0 || length <= 0 || start >= frame.size())
        return QByteArray();
    const int end = qMin(start + length, frame.size());
    return frame.mid(start, end - start);
}

QByteArray pickBits(const QByteArray& frame, int start, int length)
{
    if (start < 0 || length <= 0)
        return QByteArray();

    const int totalBits = frame.size() * 8;
    if (start >= totalBits)
        return QByteArray();

    const int end = qMin(start + length, totalBits);
    const int bitCount = end - start;
    const int byteCount = (bitCount + 7) / 8;

    QByteArray picked(byteCount, '\0');

    for (int i = 0; i < bitCount; ++i) {
        const int srcBit = start + i;
        const int byteIdx = srcBit / 8;
        const int bitIdx = 7 - (srcBit % 8); // MSB-first bit order

        const unsigned char src = static_cast<unsigned char>(frame.at(byteIdx));
        if ((src >> bitIdx) & 1) {
            const int dstByte = i / 8;
            const int dstBit = 7 - (i % 8);
            picked[dstByte] = static_cast<char>(
                static_cast<unsigned char>(picked.at(dstByte)) | (1u << dstBit));
        }
    }

    return picked;
}

} // namespace

QList<QVariant> DecimalPickerMatcher::computeValues(const QByteArray& frame) const
{
    QList<QVariant> values;
    values.reserve(m_pickers.size());

    for (const Picker& picker : m_pickers) {
        QByteArray picked;
        if (picker.unit == Picker::Unit::Bytes)
            picked = pickBytes(frame, picker.start, picker.length);
        else
            picked = pickBits(frame, picker.start, picker.length);

        // Picking from bits only supports uint, always big-endian (MSB-first).
        const NumberType effectiveType =
            (picker.unit == Picker::Unit::Bits) ? NumberType::UInt : picker.numberType;
        const ByteOrder effectiveOrder =
            (picker.unit == Picker::Unit::Bits) ? ByteOrder::BigEndian : picker.byteOrder;

        values.append(DecimalConverter::convert(picked, effectiveType, effectiveOrder));
    }

    return values;
}

QList<QByteArray> DecimalPickerMatcher::process(const QByteArray& input)
{
    Q_UNUSED(input);
    // Terminal matcher: no downstream frames.
    return {};
}

} // namespace comm
