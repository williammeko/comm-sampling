#pragma once

#include <QWidget>

#include "common/Types.h"

class QComboBox;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QStackedWidget;

namespace comm {

class CommunicationSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommunicationSettingsWidget(QWidget* parent = nullptr);

    ChannelMode mode() const;
    void setMode(ChannelMode mode);

    SerialConfig serialConfig() const;
    void setSerialConfig(const SerialConfig& cfg);

    TcpClientConfig tcpClientConfig() const;
    void setTcpClientConfig(const TcpClientConfig& cfg);

    TcpServerConfig tcpServerConfig() const;
    void setTcpServerConfig(const TcpServerConfig& cfg);

    int byteCacheSize() const;
    void setByteCacheSize(int bytes);

    void setConnected(bool connected);

signals:
    void connectRequested();
    void disconnectRequested();
    void settingsChanged();

private slots:
    void onModeChanged(int index);
    void refreshPorts();
    void onToggleClicked();

private:
    void updateToggleText();

    QComboBox* m_modeCombo = nullptr;
    QStackedWidget* m_stack = nullptr;

    // Serial controls
    QComboBox* m_portCombo = nullptr;
    QComboBox* m_baudCombo = nullptr;
    QComboBox* m_dataBitsCombo = nullptr;
    QComboBox* m_parityCombo = nullptr;
    QComboBox* m_stopBitsCombo = nullptr;

    // TCP client controls
    QLineEdit* m_hostEdit = nullptr;
    QSpinBox* m_clientPortSpin = nullptr;

    // TCP server controls
    QComboBox* m_ifaceCombo = nullptr;
    QSpinBox* m_serverPortSpin = nullptr;

    QSpinBox* m_cacheSizeSpin = nullptr;
    QList<QPushButton*> m_toggleButtons;

    bool m_connected = false;
};

} // namespace comm
