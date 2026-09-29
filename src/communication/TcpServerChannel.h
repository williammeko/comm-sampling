#pragma once

#include "communication/AbstractChannel.h"
#include "common/Types.h"

#include <QTcpServer>
#include <QTcpSocket>

namespace comm {

class TcpServerChannel : public AbstractChannel
{
    Q_OBJECT

public:
    explicit TcpServerChannel(const TcpServerConfig& cfg, QObject* parent = nullptr);
    ~TcpServerChannel() override;

    bool open() override; // starts listening
    void close() override;
    bool isOpen() const override;

protected:
    qint64 doWrite(const QByteArray& data) override; // broadcast to all clients

private slots:
    void onNewConnection();

private:
    void attachSocket(QTcpSocket* socket);

    QTcpServer* m_server = nullptr;
    QList<QTcpSocket*> m_clients;
    TcpServerConfig m_cfg;
};

} // namespace comm
