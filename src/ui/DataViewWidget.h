#pragma once

#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QWidget>

class QPlainTextEdit;
class QVBoxLayout;
class QCheckBox;
class QSpinBox;

namespace comm {

// Right-side data view: a sending preview on top, then one "Matched frames"
// box per fixed-keyword-frame matcher.
class DataViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataViewWidget(QWidget* parent = nullptr);

    void setMatchers(const QStringList& titles); // rebuild "Matched frames" boxes
    void appendSent(const QByteArray& data);
    void showFrame(int matcherIndex, const QByteArray& frame);
    void clear();

private:
    QCheckBox* m_sendCheck = nullptr;
    QSpinBox* m_sendKeepSpin = nullptr;
    QPlainTextEdit* m_sendPreview = nullptr;
    QStringList m_sendLines;
    QVBoxLayout* m_framesLayout = nullptr;
    QList<QCheckBox*> m_frameChecks;
    QList<QSpinBox*> m_frameKeepSpins;
    QList<QPlainTextEdit*> m_frameViews;
    QList<QStringList> m_frameLines;
};

} // namespace comm
