#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include "common/Types.h"

namespace comm {

enum class SendStrategyType {
    None = 0,
    Crc = 1,
    DataIncreasing = 2,
    RandomData = 3,
};

enum class CrcAlgorithm {
    None = 0,
    Crc8 = 1,
    Crc16CcittFalse = 2,
    Crc16Modbus = 3,
    Crc16Xmodem = 4,
    Crc16Ibm = 5,
    Crc32 = 6,
    // Byte-reversed variants (checksum bytes appended least-significant first).
    Crc16CcittFalseReverse = 7,
    Crc16ModbusReverse = 8,
    Crc16XmodemReverse = 9,
    Crc16IbmReverse = 10,
    Crc32Reverse = 11,
};

// One field specification: replace `byteCount` bytes starting at `byteIndex`
// in the template. `byteOrder` only matters for multi-byte increasing fields.
struct FieldSpec
{
    int byteIndex = 0;
    int byteCount = 1;
    ByteOrder byteOrder = ByteOrder::BigEndian;
};

// Builds outgoing payloads from a user template according to the selected
// strategy: plain, CRC-appended, data-increasing, or random-data fields.
class SendStrategy
{
public:
    SendStrategyType type() const { return m_type; }
    void setType(SendStrategyType type);

    CrcAlgorithm crcAlgorithm() const { return m_crc; }
    void setCrcAlgorithm(CrcAlgorithm algo) { m_crc = algo; }

    // Parses "0-1; 1-2-b; 3-2-l" style field specs. Returns false when no
    // valid field could be parsed.
    bool setFieldSpec(const QString& spec);
    QString fieldSpecText() const { return m_fieldSpecText; }

    // Builds the next payload and advances the increasing counters.
    QByteArray next(const QByteArray& templateData);

    void resetCounters();

private:
    SendStrategyType m_type = SendStrategyType::None;
    CrcAlgorithm m_crc = CrcAlgorithm::None;
    QString m_fieldSpecText;
    QList<FieldSpec> m_fields;
    QList<quint64> m_counters; // one per field, for DataIncreasing
};

} // namespace comm
