#pragma once

#include <QColor>
#include <QVariant>

#include "common/Types.h"
#include "matchers/Matcher.h"

namespace comm {

// A single value extraction from a frame: a bit range or byte range plus
// how to interpret it as a number.
struct Picker
{
    enum class Unit {
        Bits,
        Bytes,
    };

    Unit unit = Unit::Bytes;
    int start = 0;
    int length = 2;
    NumberType numberType = NumberType::UInt;
    ByteOrder byteOrder = ByteOrder::BigEndian;

    QString name;  // assigned globally: val0, val1, ...
    QColor color;  // assigned globally
};

// Cuts bit/byte ranges out of each input frame and converts them to decimal
// values, one value per configured picker. This is a terminal matcher: it
// consumes frames but produces no downstream frames.
class DecimalPickerMatcher : public Matcher
{
public:
    Type type() const override { return Type::DecimalPicker; }
    QString name() const override { return QStringLiteral("Decimal-picker"); }

    QList<Picker>& pickers() { return m_pickers; }
    const QList<Picker>& pickers() const { return m_pickers; }

    void addPicker(const Picker& picker) { m_pickers.append(picker); }
    void removePickerAt(int index)
    {
        if (index >= 0 && index < m_pickers.size())
            m_pickers.removeAt(index);
    }

    // One result per picker, in picker order. Invalid QVariant when out of range.
    QList<QVariant> computeValues(const QByteArray& frame) const;

    QList<QByteArray> process(const QByteArray& input) override;

private:
    QList<Picker> m_pickers;
};

} // namespace comm
