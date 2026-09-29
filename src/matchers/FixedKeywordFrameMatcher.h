#pragma once

#include "matchers/Matcher.h"

namespace comm {

// Matches frames by searching for a fixed keyword in the incoming byte stream
// and taking `frameLength` bytes starting at the keyword. If fewer than
// `frameLength` bytes are available, it matches as many as possible without
// crashing.
class FixedKeywordFrameMatcher : public Matcher
{
public:
    FixedKeywordFrameMatcher() = default;

    Type type() const override { return Type::FixedKeywordFrame; }
    QString name() const override { return QStringLiteral("Fixed-keyword-frame"); }

    QByteArray keyword() const { return m_keyword; }
    void setKeyword(const QByteArray& keyword); // capped at 10 bytes

    int frameLength() const { return m_frameLength; }
    void setFrameLength(int length); // clamped to 0..255

    void reset();

    QList<QByteArray> process(const QByteArray& input) override;

private:
    QByteArray m_buffer;
    QByteArray m_keyword;
    int m_frameLength = 8;
};

} // namespace comm
