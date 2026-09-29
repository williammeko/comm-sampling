#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QVariant>

#include "matchers/Matcher.h"

namespace comm {

// Chains matchers. Matcher 0 consumes the raw device stream; matcher i
// consumes the frames produced by matcher i-1.
class MatcherPipeline : public QObject
{
    Q_OBJECT

public:
    explicit MatcherPipeline(QObject* parent = nullptr);
    ~MatcherPipeline() override;

    void setMatchers(const QList<Matcher*>& matchers);

    int matcherCount() const { return m_matchers.size(); }
    Matcher* matcherAt(int index) const;

    void feed(const QByteArray& data);

signals:
    void frameMatched(int matcherIndex, const QByteArray& frame);
    void pickerValueChanged(int matcherIndex, int pickerIndex, const QVariant& value, bool valid);

private:
    QList<Matcher*> m_matchers; // not owned
};

} // namespace comm
