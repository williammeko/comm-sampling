#pragma once

#include <QByteArray>
#include <QVariant>

#include "common/Types.h"

namespace comm {
namespace DecimalConverter {

// Converts picked bytes to a decimal value.
//   UInt    -> quint64 (up to 8 bytes)
//   Int     -> qint64  (signed, sign-extended from the byte width)
//   Ieee754 -> double  (4-byte float or 8-byte double)
// Returns an invalid QVariant when conversion is not possible.
QVariant convert(const QByteArray& bytes, NumberType type, ByteOrder order);

} // namespace DecimalConverter
} // namespace comm
