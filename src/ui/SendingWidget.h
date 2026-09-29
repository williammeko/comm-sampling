#pragma once

#include <QByteArray>
#include <QWidget>

#include "sender/DataSender.h"

class QComboBox;
class QCheckBox;
class QSpinBox;
class QPushButton;

namespace comm {

class SendingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SendingWidget(DataSender* sender, QWidget* parent = nullptr);

    void setConnected(bool connected);
    void setLooping(bool looping);

signals:
    void sendRequested(const QByteArray& data, bool loop, int intervalMs);
    void stopRequested();

private slots:
    void onSendClicked();
    void onLoopToggled(bool checked);
    void onHistoryChanged();

private:
    void refreshButtons();

    DataSender* m_sender = nullptr;
    QComboBox* m_inputCombo = nullptr;
    QCheckBox* m_loopCheck = nullptr;
    QSpinBox* m_intervalSpin = nullptr;
    QPushButton* m_sendButton = nullptr;
    QPushButton* m_stopButton = nullptr;

    bool m_connected = false;
    bool m_looping = false;
};

} // namespace comm
