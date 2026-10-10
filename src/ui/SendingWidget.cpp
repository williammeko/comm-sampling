#include "SendingWidget.h"

#include "common/HexUtils.h"
#include "ui/RawDataWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <QtGlobal>

namespace comm {

SendingWidget::SendingWidget(DataSender* sender, QWidget* parent)
    : QWidget(parent)
    , m_sender(sender)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    // Row 1: template data + loop + send/stop.
    auto* row = new QHBoxLayout;

    row->addWidget(new QLabel(tr("Template data (hex):")));
    m_inputCombo = new QComboBox(this);
    m_inputCombo->setEditable(true);
    m_inputCombo->setInsertPolicy(QComboBox::NoInsert);
    m_inputCombo->setMinimumWidth(200);
    row->addWidget(m_inputCombo, 1);

    m_loopCheck = new QCheckBox(tr("Loop"), this);
    m_intervalSpin = new QSpinBox(this);
    m_intervalSpin->setRange(10, 3600000);
    m_intervalSpin->setValue(1000);
    m_intervalSpin->setSuffix(tr(" ms"));
    m_intervalSpin->setVisible(false);
    m_sendButton = new QPushButton(tr("Send"), this);
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_stopButton->setEnabled(false);
    m_sendFileButton = new QPushButton(tr("Send file"), this);

    row->addWidget(m_loopCheck);
    row->addWidget(m_intervalSpin);
    row->addWidget(m_sendButton);
    row->addWidget(m_stopButton);
    row->addWidget(m_sendFileButton);
    layout->addLayout(row);

    // Row 2: strategy + crc algorithm + field spec.
    auto* strategyRow = new QHBoxLayout;

    strategyRow->addWidget(new QLabel(tr("Strategy:")));
    m_strategyCombo = new QComboBox(this);
    m_strategyCombo->addItem(tr("none"), static_cast<int>(SendStrategyType::None));
    m_strategyCombo->addItem(tr("crc"), static_cast<int>(SendStrategyType::Crc));
    m_strategyCombo->addItem(tr("data-increasing"), static_cast<int>(SendStrategyType::DataIncreasing));
    m_strategyCombo->addItem(tr("random-data"), static_cast<int>(SendStrategyType::RandomData));
    strategyRow->addWidget(m_strategyCombo);

    m_crcLabel = new QLabel(tr("CRC:"), this);
    strategyRow->addWidget(m_crcLabel);
    m_crcCombo = new QComboBox(this);
    m_crcCombo->addItem(tr("non-crc"), static_cast<int>(CrcAlgorithm::None));
    m_crcCombo->addItem(QStringLiteral("CRC-8"), static_cast<int>(CrcAlgorithm::Crc8));
    m_crcCombo->addItem(QStringLiteral("CRC-16/CCITT-FALSE"), static_cast<int>(CrcAlgorithm::Crc16CcittFalse));
    m_crcCombo->addItem(QStringLiteral("CRC-16/CCITT-FALSE (reverse)"), static_cast<int>(CrcAlgorithm::Crc16CcittFalseReverse));
    m_crcCombo->addItem(QStringLiteral("CRC-16/MODBUS"), static_cast<int>(CrcAlgorithm::Crc16Modbus));
    m_crcCombo->addItem(QStringLiteral("CRC-16/MODBUS (reverse)"), static_cast<int>(CrcAlgorithm::Crc16ModbusReverse));
    m_crcCombo->addItem(QStringLiteral("CRC-16/XMODEM"), static_cast<int>(CrcAlgorithm::Crc16Xmodem));
    m_crcCombo->addItem(QStringLiteral("CRC-16/XMODEM (reverse)"), static_cast<int>(CrcAlgorithm::Crc16XmodemReverse));
    m_crcCombo->addItem(QStringLiteral("CRC-16/IBM"), static_cast<int>(CrcAlgorithm::Crc16Ibm));
    m_crcCombo->addItem(QStringLiteral("CRC-16/IBM (reverse)"), static_cast<int>(CrcAlgorithm::Crc16IbmReverse));
    m_crcCombo->addItem(QStringLiteral("CRC-32"), static_cast<int>(CrcAlgorithm::Crc32));
    m_crcCombo->addItem(QStringLiteral("CRC-32 (reverse)"), static_cast<int>(CrcAlgorithm::Crc32Reverse));
    strategyRow->addWidget(m_crcCombo);

    m_fieldSpecLabel = new QLabel(tr("Fields:"), this);
    strategyRow->addWidget(m_fieldSpecLabel);
    m_fieldSpecCombo = new QComboBox(this);
    m_fieldSpecCombo->setEditable(true);
    m_fieldSpecCombo->setInsertPolicy(QComboBox::NoInsert);
    m_fieldSpecCombo->setToolTip(tr("e.g. 0-1; 1-2-b; 3-2-l"));
    strategyRow->addWidget(m_fieldSpecCombo, 1);

    layout->addLayout(strategyRow);

    // Receiving preview (raw data).
    m_rawDataWidget = new RawDataWidget(this);
    layout->addWidget(m_rawDataWidget, 1);

    onHistoryChanged();
    updateStrategyUi();
    refreshButtons();

    connect(m_loopCheck, &QCheckBox::toggled, this, &SendingWidget::onLoopToggled);
    connect(m_sendButton, &QPushButton::clicked, this, &SendingWidget::onSendClicked);
    connect(m_sendFileButton, &QPushButton::clicked, this, &SendingWidget::onSendFileClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &SendingWidget::onStopClicked);
    if (m_sender)
        connect(m_sender, &DataSender::historyChanged, this, &SendingWidget::onHistoryChanged);
    connect(m_intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SendingWidget::settingsChanged);

    connect(m_strategyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SendingWidget::onStrategyChanged);
    connect(m_crcCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SendingWidget::onCrcChanged);
    connect(m_fieldSpecCombo, QOverload<int>::of(&QComboBox::activated),
            this, &SendingWidget::onFieldSpecSelected);
    connect(m_fieldSpecCombo->lineEdit(), &QLineEdit::editingFinished,
            this, &SendingWidget::onFieldSpecSubmitted);

    connect(m_rawDataWidget, &RawDataWidget::settingsChanged,
            this, &SendingWidget::settingsChanged);
}

void SendingWidget::onSendClicked()
{
    const QString text = m_inputCombo->currentText().trimmed();
    QByteArray templateData;
    if (!HexUtils::parseHexBytes(text, templateData)) {
        QMessageBox::warning(this, tr("Invalid data"),
                             tr("Enter bytes as hex, e.g. \"05 06 07 08 09 0A\"."));
        return;
    }

    if (!m_connected) {
        QMessageBox::warning(this, tr("Not connected"),
                             tr("Please connect to a device first."));
        return;
    }

    if (!m_sender)
        return;

    m_sender->addToHistory(text);

    if (m_loopCheck->isChecked()) {
        m_sender->startLoop([this, templateData]() {
            const QByteArray data = m_strategy.next(templateData);
            recordSent(data);
            return data;
        }, m_intervalSpin->value());
        m_looping = true;
        refreshButtons();
    } else {
        const QByteArray data = m_strategy.next(templateData);
        recordSent(data);
        m_sender->sendOnce(data);
    }
}

void SendingWidget::onStopClicked()
{
    if (m_sender)
        m_sender->stopLoop();
    m_looping = false;
    refreshButtons();
}

void SendingWidget::onSendFileClicked()
{
    if (!m_connected) {
        QMessageBox::warning(this, tr("Not connected"),
                             tr("Please connect to a device first."));
        return;
    }
    if (!m_sender)
        return;

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Send file"), QString(), tr("All files (*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Open failed"),
                             tr("Could not open the selected file."));
        return;
    }

    constexpr qint64 kMaxBytes = 100 * 1024; // 100 KB
    QByteArray data = file.readAll();
    file.close();

    if (data.size() > kMaxBytes) {
        const auto answer = QMessageBox::question(
            this, tr("Confirm"),
            tr("warning: only 100 kb can be sent. \nsend anyway?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
        data.truncate(static_cast<int>(kMaxBytes));
    }

    recordSent(data);
    m_sender->sendOnce(data);
}

void SendingWidget::onLoopToggled(bool checked)
{
    m_intervalSpin->setVisible(checked);
}

void SendingWidget::onHistoryChanged()
{
    if (!m_sender)
        return;

    const QString current = m_inputCombo->currentText();
    m_inputCombo->clear();
    for (const QString& item : m_sender->history())
        m_inputCombo->addItem(item);
    m_inputCombo->setEditText(current);
}

void SendingWidget::onStrategyChanged(int index)
{
    Q_UNUSED(index);
    m_strategy.setType(static_cast<SendStrategyType>(m_strategyCombo->currentData().toInt()));
    updateStrategyUi();
    emit settingsChanged();
}

void SendingWidget::onCrcChanged(int index)
{
    Q_UNUSED(index);
    m_strategy.setCrcAlgorithm(static_cast<CrcAlgorithm>(m_crcCombo->currentData().toInt()));
    emit settingsChanged();
}

void SendingWidget::onFieldSpecSubmitted()
{
    if (m_syncingFields)
        return;

    const QString text = m_fieldSpecCombo->currentText().trimmed();
    if (text.isEmpty())
        return;

    m_fieldSpecHistory.removeAll(text);
    m_fieldSpecHistory.prepend(text);
    while (m_fieldSpecHistory.size() > 50)
        m_fieldSpecHistory.removeLast();

    refreshFieldSpecItems();
    m_strategy.setFieldSpec(text);
    emit settingsChanged();
}

void SendingWidget::onFieldSpecSelected(int index)
{
    Q_UNUSED(index);
    if (m_syncingFields)
        return;
    m_strategy.setFieldSpec(m_fieldSpecCombo->currentText());
    emit settingsChanged();
}

void SendingWidget::setConnected(bool connected)
{
    m_connected = connected;
    refreshButtons();
}

void SendingWidget::setLooping(bool looping)
{
    m_looping = looping;
    refreshButtons();
}

int SendingWidget::intervalMs() const
{
    return m_intervalSpin->value();
}

void SendingWidget::setIntervalMs(int ms)
{
    m_intervalSpin->setValue(ms);
}

SendStrategyType SendingWidget::strategyType() const
{
    return m_strategy.type();
}

CrcAlgorithm SendingWidget::crcAlgorithm() const
{
    return m_strategy.crcAlgorithm();
}

QString SendingWidget::fieldSpec() const
{
    return m_fieldSpecCombo->currentText().trimmed();
}

QStringList SendingWidget::fieldSpecHistory() const
{
    return m_fieldSpecHistory;
}

void SendingWidget::setStrategyType(SendStrategyType type)
{
    m_strategy.setType(type);
    const int idx = m_strategyCombo->findData(static_cast<int>(type));
    if (idx >= 0)
        m_strategyCombo->setCurrentIndex(idx);
    updateStrategyUi();
}

void SendingWidget::setCrcAlgorithm(CrcAlgorithm algo)
{
    m_strategy.setCrcAlgorithm(algo);
    const int idx = m_crcCombo->findData(static_cast<int>(algo));
    if (idx >= 0)
        m_crcCombo->setCurrentIndex(idx);
}

void SendingWidget::setFieldSpec(const QString& spec)
{
    m_syncingFields = true;
    m_fieldSpecCombo->setEditText(spec);
    m_syncingFields = false;
    m_strategy.setFieldSpec(spec);
}

void SendingWidget::setFieldSpecHistory(const QStringList& history)
{
    m_fieldSpecHistory = history;
    refreshFieldSpecItems();
}

QString SendingWidget::templateData() const
{
    return m_inputCombo->currentText().trimmed();
}

void SendingWidget::setTemplateData(const QString& text)
{
    m_inputCombo->setEditText(text);
}

void SendingWidget::appendReceivedBytes(const QByteArray& bytes)
{
    if (m_rawDataWidget)
        m_rawDataWidget->appendBytes(bytes);
}

void SendingWidget::clearReceivedData()
{
    if (m_rawDataWidget)
        m_rawDataWidget->clear();
}

int SendingWidget::rawDataMaxBytes() const
{
    return m_rawDataWidget ? m_rawDataWidget->maxBytes() : 2000;
}

void SendingWidget::setRawDataMaxBytes(int bytes)
{
    if (m_rawDataWidget)
        m_rawDataWidget->setMaxBytes(bytes);
}

void SendingWidget::recordSent(const QByteArray& data)
{
    emit dataSent(data);
}

void SendingWidget::refreshButtons()
{
    m_sendButton->setEnabled(m_connected && !m_looping);
    m_sendFileButton->setEnabled(m_connected && !m_looping);
    m_stopButton->setEnabled(m_looping);
    m_loopCheck->setEnabled(!m_looping);
}

void SendingWidget::updateStrategyUi()
{
    const SendStrategyType type = m_strategy.type();
    const bool showCrc = (type != SendStrategyType::None);
    const bool showFields = (type == SendStrategyType::DataIncreasing
                             || type == SendStrategyType::RandomData);

    m_crcLabel->setVisible(showCrc);
    m_crcCombo->setVisible(showCrc);
    m_fieldSpecLabel->setVisible(showFields);
    m_fieldSpecCombo->setVisible(showFields);
}

void SendingWidget::refreshFieldSpecItems()
{
    m_syncingFields = true;
    const QString current = m_fieldSpecCombo->currentText();
    m_fieldSpecCombo->clear();
    for (const QString& item : m_fieldSpecHistory)
        m_fieldSpecCombo->addItem(item);
    m_fieldSpecCombo->setEditText(current);
    m_syncingFields = false;
}

} // namespace comm
