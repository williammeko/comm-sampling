#include "SendingWidget.h"

#include "common/HexUtils.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace comm {

SendingWidget::SendingWidget(DataSender* sender, QWidget* parent)
    : QWidget(parent)
    , m_sender(sender)
{
    auto* row = new QHBoxLayout(this);

    row->addWidget(new QLabel(tr("Data (hex):")));
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

    row->addWidget(m_loopCheck);
    row->addWidget(m_intervalSpin);
    row->addWidget(m_sendButton);
    row->addWidget(m_stopButton);

    onHistoryChanged();
    refreshButtons();

    connect(m_loopCheck, &QCheckBox::toggled, this, &SendingWidget::onLoopToggled);
    connect(m_sendButton, &QPushButton::clicked, this, &SendingWidget::onSendClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &SendingWidget::stopRequested);
    if (m_sender)
        connect(m_sender, &DataSender::historyChanged, this, &SendingWidget::onHistoryChanged);
}

void SendingWidget::onSendClicked()
{
    const QString text = m_inputCombo->currentText().trimmed();
    QByteArray data;
    if (!HexUtils::parseHexBytes(text, data)) {
        QMessageBox::warning(this, tr("Invalid data"),
                             tr("Enter bytes as hex, e.g. \"05 06 07 08 09 0A\"."));
        return;
    }

    if (m_sender)
        m_sender->addToHistory(text);

    emit sendRequested(data, m_loopCheck->isChecked(), m_intervalSpin->value());
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

void SendingWidget::refreshButtons()
{
    m_sendButton->setEnabled(m_connected && !m_looping);
    m_stopButton->setEnabled(m_looping);
    m_loopCheck->setEnabled(!m_looping);
}

} // namespace comm
