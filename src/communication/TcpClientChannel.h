#pragma once

#include "communication/AbstractChannel.h"
#include "common/Types.h"

#include <QTcpSocket>

namespace comm {

class TcpClientChannel : public AbstractChannel
{
    Q_OBJECT

public:
    explicit TcpClientChannel(const TcpClientConfig& cfg, QObject* parent = nullptr);
    ~TcpClientChannel() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;

protected:
    qint64 doWrite(const QByteArray& data) override;

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

private:
    QTcpSocket* m_socket = nullptr;
    TcpClientConfig m_cfg;
};

} // namespace comm
