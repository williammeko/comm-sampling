#include "SerialChannel.h"

#include <QIODevice>

namespace comm {

namespace {

QSerialPort::Parity mapParity(Parity p)
{
    switch (p) {
    case Parity::Even: return QSerialPort::EvenParity;
    case Parity::Odd: return QSerialPort::OddParity;
    case Parity::Space: return QSerialPort::SpaceParity;
    case Parity::Mark: return QSerialPort::MarkParity;
    case Parity::None:
    default: return QSerialPort::NoParity;
    }
}

QSerialPort::StopBits mapStopBits(StopBits s)
{
    switch (s) {
    case StopBits::OneAndHalf: return QSerialPort::OneAndHalfStop;
    case StopBits::Two: return QSerialPort::TwoStop;
    case StopBits::One:
    default: return QSerialPort::OneStop;
    }
}

} // namespace

SerialChannel::SerialChannel(const SerialConfig& cfg, QObject* parent)
    : AbstractChannel(parent)
    , m_cfg(cfg)
{
    m_port = new QSerialPort(this);
    connect(m_port, &QSerialPort::readyRead, this, &SerialChannel::onReadyRead);
    connect(m_port, &QSerialPort::errorOccurred, this, &SerialChannel::onError);
}

SerialChannel::~SerialChannel()
{
    close();
}

bool SerialChannel::open()
{
    if (m_port->isOpen())
        return true;

    m_port->setPortName(m_cfg.portName);
    m_port->setBaudRate(m_cfg.baudRate);
    m_port->setDataBits(static_cast<QSerialPort::DataBits>(static_cast<int>(m_cfg.dataBits)));
    m_port->setParity(mapParity(m_cfg.parity));
    m_port->setStopBits(mapStopBits(m_cfg.stopBits));

    if (!m_port->open(QIODevice::ReadWrite)) {
        emit errorOccurred(m_port->errorString());
        return false;
    }

    emit opened();
    return true;
}

void SerialChannel::close()
{
    if (m_port->isOpen()) {
        m_port->close();
        emit closed();
    }
}

bool SerialChannel::isOpen() const
{
    return m_port->isOpen();
}

qint64 SerialChannel::doWrite(const QByteArray& data)
{
    if (!m_port->isOpen())
        return -1;
    return m_port->write(data);
}

void SerialChannel::onReadyRead()
{
    const QByteArray data = m_port->readAll();
    if (!data.isEmpty())
        emit bytesReceived(data);
}

void SerialChannel::onError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;

    emit errorOccurred(m_port->errorString());

    if (m_port->isOpen()) {
        m_port->close();
        emit closed();
    }
}

} // namespace comm
