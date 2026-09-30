#include "MatcherPipeline.h"

#include "matchers/DecimalPickerMatcher.h"

namespace comm {

MatcherPipeline::MatcherPipeline(QObject* parent)
    : QObject(parent)
{
}

MatcherPipeline::~MatcherPipeline() = default;

void MatcherPipeline::setMatchers(const QList<Matcher*>& matchers)
{
    m_matchers = matchers;
}

Matcher* MatcherPipeline::matcherAt(int index) const
{
    if (index < 0 || index >= m_matchers.size())
        return nullptr;
    return m_matchers.at(index);
}

void MatcherPipeline::feed(const QByteArray& data)
{
    if (m_matchers.isEmpty())
        return;

    QList<QByteArray> frames;
    frames.append(data);

    for (int i = 0; i < m_matchers.size(); ++i) {
        Matcher* matcher = m_matchers.at(i);

        if (matcher->type() == Matcher::Type::DecimalPicker) {
            auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(matcher);
            if (!pickerMatcher)
                return;

            for (const QByteArray& frame : frames) {
                emit frameMatched(i, frame); // show the picker's input frames
                const QList<QVariant> values = pickerMatcher->computeValues(frame);
                for (int p = 0; p < values.size(); ++p)
                    emit pickerValueChanged(i, p, values.at(p), values.at(p).isValid());
            }
            return; // terminal matcher
        }

        QList<QByteArray> next;
        for (const QByteArray& frame : frames) {
            next += matcher->process(frame);
        }

        for (const QByteArray& frame : next)
            emit frameMatched(i, frame);

        frames = next;
    }
}

} // namespace comm
