#include "MatcherItemWidget.h"

#include "common/HexUtils.h"
#include "matchers/DecimalPickerMatcher.h"
#include "matchers/FixedKeywordFrameMatcher.h"

#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaType>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QTextCursor>
#include <QVBoxLayout>

#include <QtGlobal>

namespace comm {

// One bit-copy adjustment row:
// byte[srcByte][srcStart:srcEnd] -> newByte[dstByte][dstStart:dstEnd]
class AdjustmentRowWidget : public QWidget
{
    Q_OBJECT

public:
    AdjustmentRowWidget(DecimalPickerMatcher* matcher, int pickerIndex, int adjustmentIndex,
                        QWidget* parent = nullptr)
        : QWidget(parent)
        , m_matcher(matcher)
        , m_pickerIndex(pickerIndex)
        , m_adjustmentIndex(adjustmentIndex)
    {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_srcByteSpin = new QSpinBox(this);
        m_srcByteSpin->setRange(0, 1000000);
        m_srcStartSpin = new QSpinBox(this);
        m_srcStartSpin->setRange(0, 7);
        m_srcEndSpin = new QSpinBox(this);
        m_srcEndSpin->setRange(0, 7);

        m_dstByteSpin = new QSpinBox(this);
        m_dstByteSpin->setRange(0, 1000000);
        m_dstStartSpin = new QSpinBox(this);
        m_dstStartSpin->setRange(0, 7);
        m_dstEndSpin = new QSpinBox(this);
        m_dstEndSpin->setRange(0, 7);

        layout->addWidget(new QLabel(tr("byte["), this));
        layout->addWidget(m_srcByteSpin);
        layout->addWidget(new QLabel(tr("]["), this));
        layout->addWidget(m_srcStartSpin);
        layout->addWidget(new QLabel(tr(":"), this));
        layout->addWidget(m_srcEndSpin);
        layout->addWidget(new QLabel(tr("] -> newByte["), this));
        layout->addWidget(m_dstByteSpin);
        layout->addWidget(new QLabel(tr("]["), this));
        layout->addWidget(m_dstStartSpin);
        layout->addWidget(new QLabel(tr(":"), this));
        layout->addWidget(m_dstEndSpin);
        layout->addWidget(new QLabel(tr("]"), this));
        layout->addStretch(1);

        apply();

        connect(m_srcByteSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &AdjustmentRowWidget::writeBack);
        connect(m_srcStartSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &AdjustmentRowWidget::writeBack);
        connect(m_srcEndSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &AdjustmentRowWidget::writeBack);
        connect(m_dstByteSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &AdjustmentRowWidget::writeBack);
        connect(m_dstStartSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &AdjustmentRowWidget::writeBack);
        connect(m_dstEndSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &AdjustmentRowWidget::writeBack);
    }

    void apply()
    {
        const BitAdjustment& adj =
            m_matcher->pickers().at(m_pickerIndex).adjustments.at(m_adjustmentIndex);
        m_srcByteSpin->setValue(adj.srcByte);
        m_srcStartSpin->setValue(adj.srcStartBit);
        m_srcEndSpin->setValue(adj.srcEndBit);
        m_dstByteSpin->setValue(adj.dstByte);
        m_dstStartSpin->setValue(adj.dstStartBit);
        m_dstEndSpin->setValue(adj.dstEndBit);
    }

private slots:
    void writeBack()
    {
        BitAdjustment& adj = m_matcher->pickers()[m_pickerIndex].adjustments[m_adjustmentIndex];
        adj.srcByte = m_srcByteSpin->value();
        adj.srcStartBit = m_srcStartSpin->value();
        adj.srcEndBit = m_srcEndSpin->value();
        adj.dstByte = m_dstByteSpin->value();
        adj.dstStartBit = m_dstStartSpin->value();
        adj.dstEndBit = m_dstEndSpin->value();
    }

private:
    DecimalPickerMatcher* m_matcher = nullptr;
    int m_pickerIndex = 0;
    int m_adjustmentIndex = 0;

    QSpinBox* m_srcByteSpin = nullptr;
    QSpinBox* m_srcStartSpin = nullptr;
    QSpinBox* m_srcEndSpin = nullptr;
    QSpinBox* m_dstByteSpin = nullptr;
    QSpinBox* m_dstStartSpin = nullptr;
    QSpinBox* m_dstEndSpin = nullptr;
};

// One row of a decimal-picker matcher:
// [valN] [bits/bytes/bits-adjustment] start [] len [] [type] [order] [result] [remove]
class PickerRowWidget : public QWidget
{
    Q_OBJECT

public:
    PickerRowWidget(DecimalPickerMatcher* matcher, int pickerIndex, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_matcher(matcher)
        , m_pickerIndex(pickerIndex)
    {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(2);

        auto* row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);

        m_nameLabel = new QLabel(this);
        m_nameLabel->setMinimumWidth(40);

        m_unitCombo = new QComboBox(this);
        m_unitCombo->addItem(tr("bits"), static_cast<int>(Picker::Unit::Bits));
        m_unitCombo->addItem(tr("bytes"), static_cast<int>(Picker::Unit::Bytes));
        m_unitCombo->addItem(tr("bits-adjustment"), static_cast<int>(Picker::Unit::BitsAdjustment));

        m_startSpin = new QSpinBox(this);
        m_startSpin->setRange(0, 1000000);
        m_lengthSpin = new QSpinBox(this);
        m_lengthSpin->setRange(0, 1000000);

        m_typeCombo = new QComboBox(this);
        m_typeCombo->addItem(tr("uint"), static_cast<int>(NumberType::UInt));
        m_typeCombo->addItem(tr("int"), static_cast<int>(NumberType::Int));
        m_typeCombo->addItem(tr("IEEE-754"), static_cast<int>(NumberType::Ieee754));

        m_orderCombo = new QComboBox(this);
        m_orderCombo->addItem(tr("Big-endian"), static_cast<int>(ByteOrder::BigEndian));
        m_orderCombo->addItem(tr("Little-endian"), static_cast<int>(ByteOrder::LittleEndian));

        m_resultEdit = new QLineEdit(this);
        m_resultEdit->setReadOnly(true);
        m_resultEdit->setPlaceholderText(tr("decimal"));
        m_resultEdit->setMinimumWidth(90);

        auto* removeButton = new QPushButton(tr("Remove"), this);

        m_startLabel = new QLabel(tr("start"), this);
        m_lenLabel = new QLabel(tr("len"), this);

        row->addWidget(m_nameLabel);
        row->addWidget(m_unitCombo);
        row->addWidget(m_startLabel);
        row->addWidget(m_startSpin);
        row->addWidget(m_lenLabel);
        row->addWidget(m_lengthSpin);
        row->addWidget(m_typeCombo);
        row->addWidget(m_orderCombo);
        row->addWidget(m_resultEdit, 1);
        row->addWidget(removeButton);
        mainLayout->addLayout(row);

        // Bits-adjustment section: byte order + [+][-] + adjustment rows.
        m_adjustmentContainer = new QWidget(this);
        auto* adjustmentLayout = new QVBoxLayout(m_adjustmentContainer);
        adjustmentLayout->setContentsMargins(12, 0, 0, 0);
        adjustmentLayout->setSpacing(2);

        auto* buttonRow = new QHBoxLayout;
        m_addButton = new QPushButton(tr("+"), m_adjustmentContainer);
        m_removeButton = new QPushButton(tr("-"), m_adjustmentContainer);
        m_addButton->setFixedWidth(28);
        m_removeButton->setFixedWidth(28);
        buttonRow->addWidget(m_addButton);
        buttonRow->addWidget(m_removeButton);
        buttonRow->addStretch(1);
        adjustmentLayout->addLayout(buttonRow);

        m_adjustmentRowsLayout = new QVBoxLayout;
        m_adjustmentRowsLayout->setContentsMargins(0, 0, 0, 0);
        adjustmentLayout->addLayout(m_adjustmentRowsLayout);

        mainLayout->addWidget(m_adjustmentContainer);

        applyPicker();

        connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &PickerRowWidget::onUnitChanged);
        connect(m_startSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &PickerRowWidget::onStartChanged);
        connect(m_lengthSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &PickerRowWidget::onLengthChanged);
        connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &PickerRowWidget::onTypeChanged);
        connect(m_orderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &PickerRowWidget::onOrderChanged);
        connect(m_addButton, &QPushButton::clicked, this, &PickerRowWidget::addAdjustment);
        connect(m_removeButton, &QPushButton::clicked, this, &PickerRowWidget::removeLastAdjustment);
        connect(removeButton, &QPushButton::clicked, this, [this]() {
            emit removeRequested(m_pickerIndex);
        });
    }

    void applyPicker()
    {
        const Picker& picker = m_matcher->pickers().at(m_pickerIndex);
        m_unitCombo->setCurrentIndex(m_unitCombo->findData(static_cast<int>(picker.unit)));
        m_startSpin->setValue(picker.start);
        m_lengthSpin->setValue(picker.length);
        m_typeCombo->setCurrentIndex(m_typeCombo->findData(static_cast<int>(picker.numberType)));
        m_orderCombo->setCurrentIndex(m_orderCombo->findData(static_cast<int>(picker.byteOrder)));
        m_nameLabel->setText(picker.name);
        setNameColor(picker.color);
        rebuildAdjustmentRows();
        updateVisibility();
        m_resultEdit->clear();
    }

    void setNameColor(const QColor& color)
    {
        m_color = color;
        m_nameLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(color.name()));
        m_resultEdit->setStyleSheet(QStringLiteral("color: %1; background: white;").arg(color.name()));
    }

    void setResult(const QVariant& value, bool valid)
    {
        if (!valid) {
            m_resultEdit->setText(QStringLiteral("n/a"));
            return;
        }

        if (value.typeId() == QMetaType::LongLong)
            m_resultEdit->setText(QString::number(value.toLongLong()));
        else if (value.typeId() == QMetaType::ULongLong)
            m_resultEdit->setText(QString::number(value.toULongLong()));
        else
            m_resultEdit->setText(QString::number(value.toDouble(), 'g', 15));
    }

signals:
    void removeRequested(int pickerIndex);

private slots:
    void onUnitChanged(int)
    {
        m_matcher->pickers()[m_pickerIndex].unit =
            static_cast<Picker::Unit>(m_unitCombo->currentData().toInt());
        rebuildAdjustmentRows();
        updateVisibility();
        m_resultEdit->clear();
    }

    void onStartChanged(int value) { m_matcher->pickers()[m_pickerIndex].start = value; }
    void onLengthChanged(int value) { m_matcher->pickers()[m_pickerIndex].length = value; }

    void onTypeChanged(int)
    {
        m_matcher->pickers()[m_pickerIndex].numberType =
            static_cast<NumberType>(m_typeCombo->currentData().toInt());
    }

    void onOrderChanged(int)
    {
        m_matcher->pickers()[m_pickerIndex].byteOrder =
            static_cast<ByteOrder>(m_orderCombo->currentData().toInt());
    }

    void addAdjustment()
    {
        m_matcher->pickers()[m_pickerIndex].adjustments.append(BitAdjustment());
        rebuildAdjustmentRows();
        m_resultEdit->clear();
    }

    void removeLastAdjustment()
    {
        QList<BitAdjustment>& adjustments = m_matcher->pickers()[m_pickerIndex].adjustments;
        if (!adjustments.isEmpty())
            adjustments.removeLast();
        rebuildAdjustmentRows();
        m_resultEdit->clear();
    }

private:
    void rebuildAdjustmentRows()
    {
        for (AdjustmentRowWidget* row : m_adjustmentRows) {
            m_adjustmentRowsLayout->removeWidget(row);
            delete row;
        }
        m_adjustmentRows.clear();

        const auto& adjustments = m_matcher->pickers().at(m_pickerIndex).adjustments;
        for (int i = 0; i < adjustments.size(); ++i) {
            auto* row = new AdjustmentRowWidget(m_matcher, m_pickerIndex, i, m_adjustmentContainer);
            m_adjustmentRows.append(row);
            m_adjustmentRowsLayout->addWidget(row);
        }
    }

    void updateVisibility()
    {
        const Picker::Unit unit = static_cast<Picker::Unit>(m_unitCombo->currentData().toInt());
        const bool bits = (unit == Picker::Unit::Bits);
        const bool bytes = (unit == Picker::Unit::Bytes);
        const bool adjust = (unit == Picker::Unit::BitsAdjustment);

        m_startLabel->setVisible(bits || bytes);
        m_startSpin->setVisible(bits || bytes);
        m_lenLabel->setVisible(bits || bytes);
        m_lengthSpin->setVisible(bits || bytes);

        m_typeCombo->setVisible(bytes);
        m_orderCombo->setVisible(bytes || adjust);

        m_adjustmentContainer->setVisible(adjust);
    }

    DecimalPickerMatcher* m_matcher = nullptr;
    int m_pickerIndex = 0;
    QColor m_color;

    QLabel* m_nameLabel = nullptr;
    QComboBox* m_unitCombo = nullptr;
    QLabel* m_startLabel = nullptr;
    QSpinBox* m_startSpin = nullptr;
    QLabel* m_lenLabel = nullptr;
    QSpinBox* m_lengthSpin = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QComboBox* m_orderCombo = nullptr;
    QLineEdit* m_resultEdit = nullptr;

    QWidget* m_adjustmentContainer = nullptr;
    QPushButton* m_addButton = nullptr;
    QPushButton* m_removeButton = nullptr;
    QVBoxLayout* m_adjustmentRowsLayout = nullptr;
    QList<AdjustmentRowWidget*> m_adjustmentRows;
};

MatcherItemWidget::MatcherItemWidget(Matcher* matcher, int index, QWidget* parent)
    : QWidget(parent)
    , m_matcher(matcher)
    , m_index(index)
{
    auto* group = new QGroupBox(this);
    auto* outer = new QVBoxLayout(group);
    outer->setContentsMargins(4, 4, 4, 4);

    // Title row
    auto* titleRow = new QHBoxLayout;
    m_titleLabel = new QLabel(this);
    titleRow->addWidget(m_titleLabel);
    titleRow->addStretch(1);
    auto* upButton = new QPushButton(tr("Up"), this);
    auto* downButton = new QPushButton(tr("Down"), this);
    auto* removeButton = new QPushButton(tr("Remove"), this);
    titleRow->addWidget(upButton);
    titleRow->addWidget(downButton);
    titleRow->addWidget(removeButton);
    outer->addLayout(titleRow);

    // Separator line between title and content.
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    outer->addWidget(line);

    if (auto* keywordMatcher = dynamic_cast<FixedKeywordFrameMatcher*>(m_matcher)) {
        auto* keywordRow = new QHBoxLayout;
        keywordRow->addWidget(new QLabel(tr("Keyword:"), this));
        m_keywordCombo = new QComboBox(this);
        m_keywordCombo->setEditable(true);
        m_keywordCombo->setInsertPolicy(QComboBox::NoInsert);
        m_keywordCombo->setMinimumWidth(160);
        keywordRow->addWidget(m_keywordCombo, 1);
        keywordRow->addWidget(new QLabel(tr("Len:"), this));
        m_frameLengthSpin = new QSpinBox(this);
        m_frameLengthSpin->setRange(0, 255);
        m_frameLengthSpin->setValue(8);
        keywordRow->addWidget(m_frameLengthSpin);
        keywordRow->addStretch(1);
        outer->addLayout(keywordRow);

        m_keywordCombo->setEditText(HexUtils::toHexString(keywordMatcher->keyword()));
        m_frameLengthSpin->setValue(keywordMatcher->frameLength());

        auto* resultHeader = new QHBoxLayout;
        resultHeader->addWidget(new QLabel(tr("Matched frames:"), this));
        resultHeader->addStretch(1);
        resultHeader->addWidget(new QLabel(tr("Keep:"), this));
        m_keepFramesSpin = new QSpinBox(this);
        m_keepFramesSpin->setRange(1, 10000);
        m_keepFramesSpin->setValue(100);
        resultHeader->addWidget(m_keepFramesSpin);
        outer->addLayout(resultHeader);

        m_resultEdit = new QPlainTextEdit(this);
        m_resultEdit->setReadOnly(true);
        m_resultEdit->setMinimumHeight(80);
        outer->addWidget(m_resultEdit);

        connect(m_keywordCombo, &QComboBox::editTextChanged,
                this, &MatcherItemWidget::onKeywordTextChanged);
        connect(m_keywordCombo->lineEdit(), &QLineEdit::editingFinished,
                this, &MatcherItemWidget::onKeywordSubmitted);
        connect(m_frameLengthSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &MatcherItemWidget::onFrameLengthChanged);
    } else if (auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(m_matcher)) {
        Q_UNUSED(pickerMatcher);
        m_pickerLayout = new QVBoxLayout;
        m_pickerLayout->setContentsMargins(0, 0, 0, 0);
        outer->addLayout(m_pickerLayout);

        m_addPickerButton = new QPushButton(tr("Add picker"), this);
        outer->addWidget(m_addPickerButton);

        connect(m_addPickerButton, &QPushButton::clicked, this, &MatcherItemWidget::addPicker);
        rebuildPickerRows();
    }

    updateTitle();

    connect(upButton, &QPushButton::clicked, this, &MatcherItemWidget::moveUpRequested);
    connect(downButton, &QPushButton::clicked, this, &MatcherItemWidget::moveDownRequested);
    connect(removeButton, &QPushButton::clicked, this, &MatcherItemWidget::removeRequested);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(group);
}

MatcherItemWidget::~MatcherItemWidget()
{
    delete m_matcher;
}

void MatcherItemWidget::setMatcherIndex(int index)
{
    m_index = index;
    updateTitle();
}

void MatcherItemWidget::updateTitle()
{
    if (m_titleLabel)
        m_titleLabel->setText(tr("Matcher %1 - %2").arg(m_index + 1).arg(m_matcher->name()));
}

void MatcherItemWidget::appendFrame(const QByteArray& frame)
{
    if (!m_resultEdit)
        return;

    m_frameLines.append(HexUtils::toHexString(frame));
    const int keep = m_keepFramesSpin->value();
    while (m_frameLines.size() > keep)
        m_frameLines.removeFirst();
    m_resultEdit->setPlainText(m_frameLines.join(QLatin1String("\n")));
    m_resultEdit->moveCursor(QTextCursor::End);
    m_resultEdit->verticalScrollBar()->setValue(m_resultEdit->verticalScrollBar()->maximum());
}

void MatcherItemWidget::updatePickerValue(int pickerIndex, const QVariant& value, bool valid)
{
    if (pickerIndex >= 0 && pickerIndex < m_pickerRows.size())
        m_pickerRows.at(pickerIndex)->setResult(value, valid);
}

void MatcherItemWidget::refreshPickerPresentation()
{
    auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(m_matcher);
    if (!pickerMatcher)
        return;

    const int count = qMin(m_pickerRows.size(), pickerMatcher->pickers().size());
    for (int i = 0; i < count; ++i)
        m_pickerRows.at(i)->applyPicker();
}

void MatcherItemWidget::addPicker()
{
    auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(m_matcher);
    if (!pickerMatcher)
        return;

    pickerMatcher->addPicker(Picker());
    rebuildPickerRows();
    emit structureChanged();
}

void MatcherItemWidget::removePicker(int pickerIndex)
{
    auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(m_matcher);
    if (!pickerMatcher)
        return;

    pickerMatcher->removePickerAt(pickerIndex);
    rebuildPickerRows();
    emit structureChanged();
}

void MatcherItemWidget::rebuildPickerRows()
{
    auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(m_matcher);
    if (!pickerMatcher)
        return;

    for (PickerRowWidget* row : m_pickerRows) {
        m_pickerLayout->removeWidget(row);
        delete row;
    }
    m_pickerRows.clear();

    const auto& pickers = pickerMatcher->pickers();
    for (int i = 0; i < pickers.size(); ++i) {
        auto* row = new PickerRowWidget(pickerMatcher, i, this);
        connect(row, &PickerRowWidget::removeRequested, this, &MatcherItemWidget::removePicker);
        m_pickerRows.append(row);
        m_pickerLayout->addWidget(row);
    }
}

void MatcherItemWidget::refreshKeywordItems(const QStringList& history)
{
    if (!m_keywordCombo)
        return;

    m_syncingKeyword = true;
    const QString current = m_keywordCombo->currentText();
    m_keywordCombo->clear();
    for (const QString& item : history)
        m_keywordCombo->addItem(item);
    m_keywordCombo->setEditText(current);
    m_syncingKeyword = false;
}

void MatcherItemWidget::onKeywordTextChanged(const QString& text)
{
    if (m_syncingKeyword)
        return;

    if (auto* keywordMatcher = dynamic_cast<FixedKeywordFrameMatcher*>(m_matcher)) {
        QByteArray keyword;
        HexUtils::parseHexBytes(text, keyword);
        keywordMatcher->setKeyword(keyword);
    }
}

void MatcherItemWidget::onKeywordSubmitted()
{
    if (m_syncingKeyword)
        return;
    emit keywordUsed(m_keywordCombo->currentText().trimmed());
}

void MatcherItemWidget::onFrameLengthChanged(int length)
{
    if (auto* keywordMatcher = dynamic_cast<FixedKeywordFrameMatcher*>(m_matcher))
        keywordMatcher->setFrameLength(length);
}

} // namespace comm

#include "MatcherItemWidget.moc"
