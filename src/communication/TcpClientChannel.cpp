#include "TcpClientChannel.h"

#include <QDebug>
#include <QNetworkProxy>

namespace comm {

TcpClientChannel::TcpClientChannel(const TcpClientConfig& cfg, QObject* parent)
    : AbstractChannel(parent)
    , m_cfg(cfg)
{
    m_socket = new QTcpSocket(this);
    // Device connections must go direct; never route through a system proxy.
    m_socket->setProxy(QNetworkProxy::NoProxy);
    connect(m_socket, &QTcpSocket::connected, this, &TcpClientChannel::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClientChannel::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClientChannel::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpClientChannel::onError);
}

TcpClientChannel::~TcpClientChannel()
{
    close();
}

bool TcpClientChannel::open()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState
        || m_socket->state() == QAbstractSocket::ConnectingState) {
        return true;
    }

    qWarning().noquote() << "TcpClientChannel: connecting to" << m_cfg.host << ":" << m_cfg.port;
    m_socket->connectToHost(m_cfg.host, m_cfg.port);
    return true;
}

void TcpClientChannel::close()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
        emit closed();
    }
}

bool TcpClientChannel::isOpen() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

qint64 TcpClientChannel::doWrite(const QByteArray& data)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState)
        return -1;
    return m_socket->write(data);
}

void TcpClientChannel::onConnected()
{
    qWarning().noquote() << "TcpClientChannel: connected to" << m_cfg.host << ":" << m_cfg.port;
    emit opened();
}

void TcpClientChannel::onDisconnected()
{
    emit closed();
}

void TcpClientChannel::onReadyRead()
{
    const QByteArray data = m_socket->readAll();
    if (!data.isEmpty())
        emit bytesReceived(data);
}

void TcpClientChannel::onError(QAbstractSocket::SocketError error)
{
    qWarning().noquote() << "TcpClientChannel error:" << int(error) << m_socket->errorString()
                         << "state=" << int(m_socket->state());
    const QString message = QStringLiteral("TCP connect to %1:%2 failed: %3")
                                .arg(m_cfg.host)
                                .arg(m_cfg.port)
                                .arg(m_socket->errorString());
    emit errorOccurred(message);
    if (m_socket->state() == QAbstractSocket::UnconnectedState)
        emit closed();
}

} // namespace comm
