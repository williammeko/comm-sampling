#pragma once

#include <QByteArray>
#include <QString>

namespace comm {
namespace HexUtils {

// Parses a hex byte string such as "05 06 07 08 09 0A 0B 0C 0D 0F".
// Accepted separators: spaces, commas, colons, semicolons, dashes, underscores.
// Optional "0x"/"0X" prefixes are also accepted.
// Returns false (and sets `errorMessage`) on invalid input.
bool parseHexBytes(const QString& text, QByteArray& out, QString* errorMessage = nullptr);

// Formats bytes as uppercase hex, e.g. "05 06 07 0A".
QString toHexString(const QByteArray& bytes, QChar separator = QLatin1Char(' '));

} // namespace HexUtils
} // namespace comm
