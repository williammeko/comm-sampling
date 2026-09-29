#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace comm {

// Base class for a data matcher. Each matcher consumes an input chunk
// (raw device stream for matcher 0, or frames from the previous matcher)
// and produces a list of matched frames.
class Matcher
{
public:
    enum class Type {
        FixedKeywordFrame,
        DecimalPicker,
    };

    virtual ~Matcher() = default;

    virtual Type type() const = 0;
    virtual QString name() const = 0;
    virtual QList<QByteArray> process(const QByteArray& input) = 0;
};

} // namespace comm
