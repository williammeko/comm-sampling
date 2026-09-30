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

QByteArray applyAdjustments(const QByteArray& frame, const QList<BitAdjustment>& adjustments)
{
    if (frame.isEmpty() || adjustments.isEmpty())
        return QByteArray();

    int dstByteCount = 0;
    for (const BitAdjustment& adj : adjustments)
        dstByteCount = qMax(dstByteCount, adj.dstByte + 1);

    QByteArray result(dstByteCount, '\0');

    for (const BitAdjustment& adj : adjustments) {
        if (adj.srcByte < 0 || adj.srcByte >= frame.size())
            continue;
        if (adj.dstByte < 0 || adj.dstByte >= dstByteCount)
            continue;

        const int srcDir = (adj.srcStartBit <= adj.srcEndBit) ? 1 : -1;
        const int srcCount = qAbs(adj.srcEndBit - adj.srcStartBit) + 1;
        const int dstDir = (adj.dstStartBit <= adj.dstEndBit) ? 1 : -1;
        const int dstCount = qAbs(adj.dstEndBit - adj.dstStartBit) + 1;
        const int count = qMin(srcCount, dstCount);

        const unsigned char src = static_cast<unsigned char>(frame.at(adj.srcByte));

        for (int i = 0; i < count; ++i) {
            const int srcBit = adj.srcStartBit + i * srcDir;
            const int dstBit = adj.dstStartBit + i * dstDir;
            if (srcBit < 0 || srcBit > 7 || dstBit < 0 || dstBit > 7)
                continue;

            if ((src >> srcBit) & 1) {
                unsigned char dst = static_cast<unsigned char>(result.at(adj.dstByte));
                dst = static_cast<unsigned char>(dst | (1u << dstBit));
                result[adj.dstByte] = static_cast<char>(dst);
            }
        }
    }

    return result;
}

} // namespace

QList<QVariant> DecimalPickerMatcher::computeValues(const QByteArray& frame) const
{
    QList<QVariant> values;
    values.reserve(m_pickers.size());

    for (const Picker& picker : m_pickers) {
        QByteArray picked;
        NumberType effectiveType = picker.numberType;
        ByteOrder effectiveOrder = picker.byteOrder;

        if (picker.unit == Picker::Unit::Bytes) {
            picked = pickBytes(frame, picker.start, picker.length);
        } else if (picker.unit == Picker::Unit::Bits) {
            picked = pickBits(frame, picker.start, picker.length);
            // Picking from bits only supports uint, always big-endian (MSB-first).
            effectiveType = NumberType::UInt;
            effectiveOrder = ByteOrder::BigEndian;
        } else { // BitsAdjustment
            picked = applyAdjustments(frame, picker.adjustments);
            effectiveType = NumberType::UInt;
        }

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
