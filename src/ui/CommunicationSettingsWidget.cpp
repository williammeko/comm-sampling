#include "CommunicationSettingsWidget.h"

#include <QAbstractSocket>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace comm {

CommunicationSettingsWidget::CommunicationSettingsWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    // Row 1: method + byte cache size.
    auto* methodRow = new QHBoxLayout;
    methodRow->addWidget(new QLabel(tr("Method:")));
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(tr("Serial Port"), static_cast<int>(ChannelMode::Serial));
    m_modeCombo->addItem(tr("TCP Client"), static_cast<int>(ChannelMode::TcpClient));
    m_modeCombo->addItem(tr("TCP Server"), static_cast<int>(ChannelMode::TcpServer));
    methodRow->addWidget(m_modeCombo);
    methodRow->addSpacing(12);
    methodRow->addWidget(new QLabel(tr("Byte cache size:")));
    m_cacheSizeSpin = new QSpinBox(this);
    m_cacheSizeSpin->setRange(256, 10000000);
    m_cacheSizeSpin->setValue(10000);
    methodRow->addWidget(m_cacheSizeSpin);
    methodRow->addStretch(1);
    outer->addLayout(methodRow);

    m_stack = new QStackedWidget(this);

    // ---- Serial page (one line) ----
    auto* serialPage = new QWidget(this);
    auto* serialRow = new QHBoxLayout(serialPage);
    serialRow->setContentsMargins(0, 0, 0, 0);
    serialRow->addWidget(new QLabel(tr("Port:")));
    m_portCombo = new QComboBox(serialPage);
    serialRow->addWidget(m_portCombo, 1);
    auto* refreshButton = new QPushButton(tr("Refresh"), serialPage);
    serialRow->addWidget(refreshButton);
    serialRow->addWidget(new QLabel(tr("Baud:")));
    m_baudCombo = new QComboBox(serialPage);
    m_baudCombo->setEditable(true);
    m_baudCombo->addItems({QStringLiteral("9600"), QStringLiteral("19200"),
                           QStringLiteral("38400"), QStringLiteral("57600"),
                           QStringLiteral("115200"), QStringLiteral("230400"),
                           QStringLiteral("460800"), QStringLiteral("921600")});
    m_baudCombo->setCurrentText(QStringLiteral("115200"));
    serialRow->addWidget(m_baudCombo);
    serialRow->addWidget(new QLabel(tr("Bits:")));
    m_dataBitsCombo = new QComboBox(serialPage);
    m_dataBitsCombo->addItems({QStringLiteral("5"), QStringLiteral("6"),
                               QStringLiteral("7"), QStringLiteral("8")});
    m_dataBitsCombo->setCurrentText(QStringLiteral("8"));
    serialRow->addWidget(m_dataBitsCombo);
    serialRow->addWidget(new QLabel(tr("Parity:")));
    m_parityCombo = new QComboBox(serialPage);
    m_parityCombo->addItems({tr("None"), tr("Even"), tr("Odd"), tr("Space"), tr("Mark")});
    serialRow->addWidget(m_parityCombo);
    serialRow->addWidget(new QLabel(tr("Stop:")));
    m_stopBitsCombo = new QComboBox(serialPage);
    m_stopBitsCombo->addItems({QStringLiteral("1"), QStringLiteral("1.5"), QStringLiteral("2")});
    serialRow->addWidget(m_stopBitsCombo);
    auto* serialToggle = new QPushButton(tr("Connect"), serialPage);
    serialRow->addWidget(serialToggle);
    m_toggleButtons.append(serialToggle);
    m_stack->addWidget(serialPage);

    // ---- TCP client page (one line) ----
    auto* clientPage = new QWidget(this);
    auto* clientRow = new QHBoxLayout(clientPage);
    clientRow->setContentsMargins(0, 0, 0, 0);
    clientRow->addWidget(new QLabel(tr("IP/Name:")));
    m_hostEdit = new QLineEdit(clientPage);
    m_hostEdit->setPlaceholderText(tr("192.168.1.10 or device.local"));
    clientRow->addWidget(m_hostEdit, 1);
    clientRow->addWidget(new QLabel(tr("Port:")));
    m_clientPortSpin = new QSpinBox(clientPage);
    m_clientPortSpin->setRange(1, 65535);
    m_clientPortSpin->setValue(8080);
    clientRow->addWidget(m_clientPortSpin);
    auto* clientToggle = new QPushButton(tr("Connect"), clientPage);
    clientRow->addWidget(clientToggle);
    m_toggleButtons.append(clientToggle);
    m_stack->addWidget(clientPage);

    // ---- TCP server page (one line) ----
    auto* serverPage = new QWidget(this);
    auto* serverRow = new QHBoxLayout(serverPage);
    serverRow->setContentsMargins(0, 0, 0, 0);
    serverRow->addWidget(new QLabel(tr("Interface:")));
    m_ifaceCombo = new QComboBox(serverPage);
    m_ifaceCombo->addItem(tr("ANY-INTERFACE (0.0.0.0)"), QStringLiteral("0.0.0.0"));
    const auto addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback())
            m_ifaceCombo->addItem(address.toString(), address.toString());
    }
    serverRow->addWidget(m_ifaceCombo, 1);
    serverRow->addWidget(new QLabel(tr("Port:")));
    m_serverPortSpin = new QSpinBox(serverPage);
    m_serverPortSpin->setRange(1, 65535);
    m_serverPortSpin->setValue(8080);
    serverRow->addWidget(m_serverPortSpin);
    auto* serverToggle = new QPushButton(tr("Listen"), serverPage);
    serverRow->addWidget(serverToggle);
    m_toggleButtons.append(serverToggle);
    m_stack->addWidget(serverPage);

    outer->addWidget(m_stack);

    refreshPorts();
    onModeChanged(0);

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CommunicationSettingsWidget::onModeChanged);
    connect(refreshButton, &QPushButton::clicked,
            this, &CommunicationSettingsWidget::refreshPorts);

    auto markChanged = [this] { emit settingsChanged(); };
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [markChanged](int) { markChanged(); });
    connect(m_portCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [markChanged](int) { markChanged(); });
    connect(m_baudCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [markChanged](int) { markChanged(); });
    connect(m_baudCombo, &QComboBox::editTextChanged,
            this, [markChanged](const QString&) { markChanged(); });
    connect(m_dataBitsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [markChanged](int) { markChanged(); });
    connect(m_parityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [markChanged](int) { markChanged(); });
    connect(m_stopBitsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [markChanged](int) { markChanged(); });
    connect(m_hostEdit, &QLineEdit::textEdited,
            this, [markChanged](const QString&) { markChanged(); });
    connect(m_clientPortSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [markChanged](int) { markChanged(); });
    connect(m_ifaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [markChanged](int) { markChanged(); });
    connect(m_serverPortSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [markChanged](int) { markChanged(); });
    connect(m_cacheSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [markChanged](int) { markChanged(); });

    for (QPushButton* button : m_toggleButtons)
        connect(button, &QPushButton::clicked, this, &CommunicationSettingsWidget::onToggleClicked);
}

ChannelMode CommunicationSettingsWidget::mode() const
{
    return static_cast<ChannelMode>(m_modeCombo->currentData().toInt());
}

void CommunicationSettingsWidget::setMode(ChannelMode mode)
{
    const int idx = m_modeCombo->findData(static_cast<int>(mode));
    if (idx >= 0)
        m_modeCombo->setCurrentIndex(idx);
}

SerialConfig CommunicationSettingsWidget::serialConfig() const
{
    SerialConfig cfg;
    cfg.portName = m_portCombo->currentData().toString();
    cfg.baudRate = m_baudCombo->currentText().toInt();
    cfg.dataBits = static_cast<DataBits>(m_dataBitsCombo->currentText().toInt());

    switch (m_parityCombo->currentIndex()) {
    case 1: cfg.parity = Parity::Even; break;
    case 2: cfg.parity = Parity::Odd; break;
    case 3: cfg.parity = Parity::Space; break;
    case 4: cfg.parity = Parity::Mark; break;
    default: cfg.parity = Parity::None; break;
    }

    switch (m_stopBitsCombo->currentIndex()) {
    case 1: cfg.stopBits = StopBits::OneAndHalf; break;
    case 2: cfg.stopBits = StopBits::Two; break;
    default: cfg.stopBits = StopBits::One; break;
    }

    return cfg;
}

void CommunicationSettingsWidget::setSerialConfig(const SerialConfig& cfg)
{
    m_baudCombo->setCurrentText(QString::number(cfg.baudRate));
    m_dataBitsCombo->setCurrentText(QString::number(static_cast<int>(cfg.dataBits)));

    int parity = 0;
    switch (cfg.parity) {
    case Parity::Even: parity = 1; break;
    case Parity::Odd: parity = 2; break;
    case Parity::Space: parity = 3; break;
    case Parity::Mark: parity = 4; break;
    default: parity = 0; break;
    }
    m_parityCombo->setCurrentIndex(parity);

    int stop = 0;
    switch (cfg.stopBits) {
    case StopBits::OneAndHalf: stop = 1; break;
    case StopBits::Two: stop = 2; break;
    default: stop = 0; break;
    }
    m_stopBitsCombo->setCurrentIndex(stop);

    int idx = m_portCombo->findData(cfg.portName);
    if (idx >= 0) {
        m_portCombo->setCurrentIndex(idx);
    } else if (!cfg.portName.isEmpty()) {
        m_portCombo->addItem(cfg.portName, cfg.portName);
        m_portCombo->setCurrentIndex(m_portCombo->count() - 1);
    }
}

TcpClientConfig CommunicationSettingsWidget::tcpClientConfig() const
{
    TcpClientConfig cfg;
    cfg.host = m_hostEdit->text().trimmed();
    cfg.port = static_cast<quint16>(m_clientPortSpin->value());
    return cfg;
}

void CommunicationSettingsWidget::setTcpClientConfig(const TcpClientConfig& cfg)
{
    m_hostEdit->setText(cfg.host);
    m_clientPortSpin->setValue(cfg.port);
}

TcpServerConfig CommunicationSettingsWidget::tcpServerConfig() const
{
    TcpServerConfig cfg;
    cfg.listenAddress = QHostAddress(m_ifaceCombo->currentData().toString());
    cfg.port = static_cast<quint16>(m_serverPortSpin->value());
    return cfg;
}

void CommunicationSettingsWidget::setTcpServerConfig(const TcpServerConfig& cfg)
{
    const int idx = m_ifaceCombo->findData(cfg.listenAddress.toString());
    if (idx >= 0)
        m_ifaceCombo->setCurrentIndex(idx);
    m_serverPortSpin->setValue(cfg.port);
}

int CommunicationSettingsWidget::byteCacheSize() const
{
    return m_cacheSizeSpin->value();
}

void CommunicationSettingsWidget::setByteCacheSize(int bytes)
{
    m_cacheSizeSpin->setValue(bytes);
}

void CommunicationSettingsWidget::onModeChanged(int)
{
    m_stack->setCurrentIndex(m_modeCombo->currentIndex());
    updateToggleText();
}

void CommunicationSettingsWidget::refreshPorts()
{
    const QString current = m_portCombo->currentData().toString();
    m_portCombo->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports) {
        QString label = info.portName();
        if (!info.description().isEmpty())
            label += QStringLiteral(" - ") + info.description();
        m_portCombo->addItem(label, info.portName());
    }

    if (!current.isEmpty()) {
        const int idx = m_portCombo->findData(current);
        if (idx >= 0)
            m_portCombo->setCurrentIndex(idx);
    }
}

void CommunicationSettingsWidget::onToggleClicked()
{
    if (m_connected)
        emit disconnectRequested();
    else
        emit connectRequested();
}

void CommunicationSettingsWidget::setConnected(bool connected)
{
    m_connected = connected;
    updateToggleText();
}

void CommunicationSettingsWidget::updateToggleText()
{
    const bool server = (mode() == ChannelMode::TcpServer);
    for (QPushButton* button : m_toggleButtons) {
        if (m_connected)
            button->setText(server ? tr("Stop") : tr("Disconnect"));
        else
            button->setText(server ? tr("Listen") : tr("Connect"));
    }
}

} // namespace comm
