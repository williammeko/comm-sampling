#pragma once

#include <QByteArray>
#include <QWidget>

#include "sender/DataSender.h"
#include "sender/SendStrategy.h"

class QComboBox;
class QCheckBox;
class QSpinBox;
class QPushButton;
class QLabel;

namespace comm {

class RawDataWidget;

class SendingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SendingWidget(DataSender* sender, QWidget* parent = nullptr);

    void setConnected(bool connected);
    void setLooping(bool looping);

    int intervalMs() const;
    void setIntervalMs(int ms);

    SendStrategyType strategyType() const;
    CrcAlgorithm crcAlgorithm() const;
    QString fieldSpec() const;
    QStringList fieldSpecHistory() const;

    void setStrategyType(SendStrategyType type);
    void setCrcAlgorithm(CrcAlgorithm algo);
    void setFieldSpec(const QString& spec);
    void setFieldSpecHistory(const QStringList& history);

    QString templateData() const;
    void setTemplateData(const QString& text);

    void appendReceivedBytes(const QByteArray& bytes);
    void clearReceivedData();
    int rawDataMaxBytes() const;
    void setRawDataMaxBytes(int bytes);

signals:
    void dataSent(const QByteArray& data);
    void settingsChanged();

private slots:
    void onSendClicked();
    void onStopClicked();
    void onLoopToggled(bool checked);
    void onHistoryChanged();
    void onStrategyChanged(int index);
    void onCrcChanged(int index);
    void onFieldSpecSubmitted();
    void onFieldSpecSelected(int index);

private:
    void refreshButtons();
    void updateStrategyUi();
    void refreshFieldSpecItems();
    void recordSent(const QByteArray& data);

    DataSender* m_sender = nullptr;
    QComboBox* m_inputCombo = nullptr;
    QCheckBox* m_loopCheck = nullptr;
    QSpinBox* m_intervalSpin = nullptr;
    QPushButton* m_sendButton = nullptr;
    QPushButton* m_stopButton = nullptr;

    QComboBox* m_strategyCombo = nullptr;
    QLabel* m_crcLabel = nullptr;
    QComboBox* m_crcCombo = nullptr;
    QLabel* m_fieldSpecLabel = nullptr;
    QComboBox* m_fieldSpecCombo = nullptr;

    RawDataWidget* m_rawDataWidget = nullptr;

    SendStrategy m_strategy;
    QStringList m_fieldSpecHistory;

    bool m_connected = false;
    bool m_looping = false;
    bool m_syncingFields = false;
};

} // namespace comm
