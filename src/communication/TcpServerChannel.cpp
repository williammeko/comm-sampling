#include "TcpServerChannel.h"

namespace comm {

TcpServerChannel::TcpServerChannel(const TcpServerConfig& cfg, QObject* parent)
    : AbstractChannel(parent)
    , m_cfg(cfg)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &TcpServerChannel::onNewConnection);
}

TcpServerChannel::~TcpServerChannel()
{
    close();
}

bool TcpServerChannel::open()
{
    if (m_server->isListening())
        return true;

    if (!m_server->listen(m_cfg.listenAddress, m_cfg.port)) {
        emit errorOccurred(m_server->errorString());
        return false;
    }

    emit opened();
    return true;
}

void TcpServerChannel::close()
{
    if (m_server->isListening())
        m_server->close();

    for (QTcpSocket* socket : m_clients) {
        socket->disconnectFromHost();
        socket->deleteLater();
    }
    m_clients.clear();

    emit closed();
}

bool TcpServerChannel::isOpen() const
{
    return m_server->isListening();
}

qint64 TcpServerChannel::doWrite(const QByteArray& data)
{
    qint64 total = 0;
    for (QTcpSocket* socket : m_clients) {
        if (socket->state() == QAbstractSocket::ConnectedState)
            total += socket->write(data);
    }
    return total;
}

void TcpServerChannel::onNewConnection()
{
    while (QTcpSocket* socket = m_server->nextPendingConnection())
        attachSocket(socket);
}

void TcpServerChannel::attachSocket(QTcpSocket* socket)
{
    socket->setParent(m_server);
    m_clients.append(socket);

    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        const QByteArray data = socket->readAll();
        if (!data.isEmpty())
            emit bytesReceived(data);
    });

    connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
        m_clients.removeAll(socket);
        socket->deleteLater();
    });
}

} // namespace comm
