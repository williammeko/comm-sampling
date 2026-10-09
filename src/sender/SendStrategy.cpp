#include "SendStrategy.h"

#include <QRandomGenerator>

namespace comm {

namespace {

quint64 reflectBits(quint64 value, int bits)
{
    quint64 result = 0;
    for (int i = 0; i < bits; ++i) {
        if (value & (1ULL << i))
            result |= (1ULL << (bits - 1 - i));
    }
    return result;
}

quint64 crcValue(const QByteArray& data, quint64 poly, quint64 init, quint64 xorOut,
                 bool reflectIn, bool reflectOut, int width)
{
    const quint64 mask = (width >= 64) ? ~0ULL : ((1ULL << width) - 1ULL);
    quint64 crc = init & mask;

    for (char c : data) {
        quint8 byte = static_cast<quint8>(c);
        if (reflectIn)
            byte = static_cast<quint8>(reflectBits(byte, 8));

        crc ^= (static_cast<quint64>(byte) << (width - 8));
        for (int i = 0; i < 8; ++i) {
            if (crc & (1ULL << (width - 1)))
                crc = ((crc << 1) ^ poly) & mask;
            else
                crc = (crc << 1) & mask;
        }
    }

    if (reflectOut)
        crc = reflectBits(crc, width);
    return (crc ^ xorOut) & mask;
}

QByteArray crcBytes(CrcAlgorithm algo, const QByteArray& data)
{
    quint64 value = 0;
    int byteCount = 0;
    bool reverse = false;

    switch (algo) {
    case CrcAlgorithm::Crc8:
        value = crcValue(data, 0x07, 0x00, 0x00, false, false, 8);
        byteCount = 1;
        break;
    case CrcAlgorithm::Crc16CcittFalse:
    case CrcAlgorithm::Crc16CcittFalseReverse:
        value = crcValue(data, 0x1021, 0xFFFF, 0x0000, false, false, 16);
        byteCount = 2;
        reverse = (algo == CrcAlgorithm::Crc16CcittFalseReverse);
        break;
    case CrcAlgorithm::Crc16Modbus:
    case CrcAlgorithm::Crc16ModbusReverse:
        value = crcValue(data, 0x8005, 0xFFFF, 0x0000, true, true, 16);
        byteCount = 2;
        reverse = (algo == CrcAlgorithm::Crc16ModbusReverse);
        break;
    case CrcAlgorithm::Crc16Xmodem:
    case CrcAlgorithm::Crc16XmodemReverse:
        value = crcValue(data, 0x1021, 0x0000, 0x0000, false, false, 16);
        byteCount = 2;
        reverse = (algo == CrcAlgorithm::Crc16XmodemReverse);
        break;
    case CrcAlgorithm::Crc16Ibm:
    case CrcAlgorithm::Crc16IbmReverse:
        value = crcValue(data, 0x8005, 0x0000, 0x0000, true, true, 16);
        byteCount = 2;
        reverse = (algo == CrcAlgorithm::Crc16IbmReverse);
        break;
    case CrcAlgorithm::Crc32:
    case CrcAlgorithm::Crc32Reverse:
        value = crcValue(data, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true, 32);
        byteCount = 4;
        reverse = (algo == CrcAlgorithm::Crc32Reverse);
        break;
    case CrcAlgorithm::None:
        return {};
    }

    QByteArray out;
    for (int i = 0; i < byteCount; ++i) {
        const int shift = reverse ? (8 * i) : (8 * (byteCount - 1 - i));
        out.append(static_cast<char>((value >> shift) & 0xFF));
    }
    return out;
}

} // namespace

void SendStrategy::setType(SendStrategyType type)
{
    m_type = type;
    resetCounters();
}

bool SendStrategy::setFieldSpec(const QString& spec)
{
    m_fields.clear();
    m_counters.clear();
    m_fieldSpecText = spec.trimmed();

    const QStringList tokens = m_fieldSpecText.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString& rawToken : tokens) {
        const QStringList parts = rawToken.trimmed().split(QLatin1Char('-'), Qt::SkipEmptyParts);
        if (parts.size() < 2 || parts.size() > 3)
            continue;

        bool ok = false;
        const int index = parts.at(0).trimmed().toInt(&ok);
        if (!ok || index < 0)
            continue;
        const int count = parts.at(1).trimmed().toInt(&ok);
        if (!ok || count < 1 || count > 8)
            continue;

        FieldSpec field;
        field.byteIndex = index;
        field.byteCount = count;
        field.byteOrder = ByteOrder::BigEndian;
        if (parts.size() == 3) {
            const QChar endian = parts.at(2).trimmed().toLower().at(0);
            if (endian == QLatin1Char('l'))
                field.byteOrder = ByteOrder::LittleEndian;
            else if (endian == QLatin1Char('b'))
                field.byteOrder = ByteOrder::BigEndian;
        }
        m_fields.append(field);
    }

    return !m_fields.isEmpty();
}

void SendStrategy::resetCounters()
{
    m_counters.clear();
    for (int i = 0; i < m_fields.size(); ++i)
        m_counters.append(0);
}

QByteArray SendStrategy::next(const QByteArray& templateData)
{
    QByteArray data = templateData;

    if (m_type == SendStrategyType::DataIncreasing) {
        while (m_counters.size() < m_fields.size())
            m_counters.append(0);

        for (int i = 0; i < m_fields.size(); ++i) {
            const FieldSpec& field = m_fields.at(i);
            if (field.byteIndex < 0 || field.byteIndex + field.byteCount > data.size())
                continue;

            const quint64 value = m_counters.at(i);
            for (int b = 0; b < field.byteCount; ++b) {
                const int index = (field.byteOrder == ByteOrder::BigEndian)
                                      ? field.byteIndex + (field.byteCount - 1 - b)
                                      : field.byteIndex + b;
                data[index] = static_cast<char>((value >> (8 * b)) & 0xFF);
            }
        }

        for (int i = 0; i < m_fields.size(); ++i) {
            const FieldSpec& field = m_fields.at(i);
            const quint64 max = (field.byteCount >= 8)
                                    ? ~0ULL
                                    : ((1ULL << (8 * field.byteCount)) - 1ULL);
            quint64 nextValue = m_counters.at(i) + 1;
            if (nextValue > max)
                nextValue = 0;
            m_counters[i] = nextValue;
        }
    } else if (m_type == SendStrategyType::RandomData) {
        auto* rng = QRandomGenerator::global();
        for (const FieldSpec& field : m_fields) {
            if (field.byteIndex < 0 || field.byteIndex + field.byteCount > data.size())
                continue;
            for (int b = 0; b < field.byteCount; ++b)
                data[field.byteIndex + b] = static_cast<char>(rng->bounded(256));
        }
    }

    if (m_type != SendStrategyType::None && m_crc != CrcAlgorithm::None)
        data += crcBytes(m_crc, data);

    return data;
}

} // namespace comm
