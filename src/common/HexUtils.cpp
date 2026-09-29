#include "HexUtils.h"

namespace comm {
namespace HexUtils {

namespace {

int hexValue(QChar c)
{
    if (c >= QLatin1Char('0') && c <= QLatin1Char('9'))
        return c.unicode() - '0';
    if (c >= QLatin1Char('a') && c <= QLatin1Char('f'))
        return c.unicode() - 'a' + 10;
    if (c >= QLatin1Char('A') && c <= QLatin1Char('F'))
        return c.unicode() - 'A' + 10;
    return -1;
}

} // namespace

bool parseHexBytes(const QString& text, QByteArray& out, QString* errorMessage)
{
    out.clear();

    QString cleaned;
    cleaned.reserve(text.size());

    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);

        // Strip "0x" / "0X" prefixes.
        if (c == QLatin1Char('0') && i + 1 < text.size()
            && (text.at(i + 1) == QLatin1Char('x') || text.at(i + 1) == QLatin1Char('X'))) {
            ++i; // skip the 'x'
            continue;
        }

        // Skip separators and whitespace.
        if (c.isSpace() || c == QLatin1Char(',') || c == QLatin1Char(':')
            || c == QLatin1Char(';') || c == QLatin1Char('-') || c == QLatin1Char('_')) {
            continue;
        }

        if (hexValue(c) < 0) {
            if (errorMessage)
                *errorMessage = QStringLiteral("Invalid character: '%1'").arg(c);
            return false;
        }

        cleaned.append(c);
    }

    if (cleaned.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Empty input");
        return false;
    }

    if (cleaned.size() % 2 != 0) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Hex string must contain an even number of digits");
        return false;
    }

    out.resize(cleaned.size() / 2);
    for (int i = 0; i < out.size(); ++i) {
        const int hi = hexValue(cleaned.at(2 * i));
        const int lo = hexValue(cleaned.at(2 * i + 1));
        out[i] = static_cast<char>((hi << 4) | lo);
    }

    return true;
}

QString toHexString(const QByteArray& bytes, QChar separator)
{
    QString s;
    s.reserve(bytes.size() * 3);
    for (int i = 0; i < bytes.size(); ++i) {
        if (i > 0)
            s.append(separator);
        const unsigned char b = static_cast<unsigned char>(bytes.at(i));
        s.append(QString::number(b >> 4, 16).toUpper());
        s.append(QString::number(b & 0xF, 16).toUpper());
    }
    return s;
}

} // namespace HexUtils
} // namespace comm
