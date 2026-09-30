#pragma once

#include <QByteArray>
#include <QStringList>
#include <QVariant>
#include <QWidget>

#include "matchers/Matcher.h"

class QComboBox;
class QLineEdit;
class QSpinBox;
class QPlainTextEdit;
class QLabel;
class QVBoxLayout;
class QPushButton;

namespace comm {

class DecimalPickerMatcher;
class PickerRowWidget;

// One matcher in the list. Fixed-keyword matchers show keyword/length and a
// matched-frames view. Decimal-picker matchers show a list of value pickers.
class MatcherItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MatcherItemWidget(Matcher* matcher, int index, QWidget* parent = nullptr);
    ~MatcherItemWidget() override;

    Matcher* matcher() const { return m_matcher; }
    int matcherIndex() const { return m_index; }
    void setMatcherIndex(int index);

    void appendFrame(const QByteArray& frame);
    void updatePickerValue(int pickerIndex, const QVariant& value, bool valid);
    void refreshPickerPresentation();
    void refreshKeywordItems(const QStringList& history);

signals:
    void structureChanged();   // a picker was added/removed
    void keywordUsed(const QString& text);
    void pickerNameChanged();
    void configChanged();      // any matcher setting changed
    void removeRequested();
    void moveUpRequested();
    void moveDownRequested();

private slots:
    void onKeywordTextChanged(const QString& text);
    void onKeywordSubmitted();
    void onFrameLengthChanged(int length);
    void addPicker();
    void removePicker(int pickerIndex);

private:
    void updateTitle();
    void rebuildPickerRows();

    Matcher* m_matcher = nullptr;
    int m_index = 0;
    QStringList m_frameLines;

    QLabel* m_titleLabel = nullptr;

    // Fixed-keyword controls
    QComboBox* m_keywordCombo = nullptr;
    QSpinBox* m_frameLengthSpin = nullptr;

    // Matched-frames view (fixed-keyword only)
    QPlainTextEdit* m_resultEdit = nullptr;
    QSpinBox* m_keepFramesSpin = nullptr;

    // Decimal-picker controls
    QVBoxLayout* m_pickerLayout = nullptr;
    QPushButton* m_addPickerButton = nullptr;
    QList<PickerRowWidget*> m_pickerRows;

    bool m_syncingKeyword = false;
};

} // namespace comm
