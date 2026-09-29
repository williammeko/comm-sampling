#pragma once

#include <QByteArray>
#include <QObject>
#include <QThread>

#include "common/Types.h"
#include "communication/AbstractChannel.h"

namespace comm {

// Owns the currently active channel and its dedicated I/O thread.
class ChannelManager : public QObject
{
    Q_OBJECT

public:
    explicit ChannelManager(QObject* parent = nullptr);
    ~ChannelManager() override;

    void startSerial(const SerialConfig& cfg);
    void startTcpClient(const TcpClientConfig& cfg);
    void startTcpServer(const TcpServerConfig& cfg);
    void stop();

    bool isOpen() const { return m_open; }
    ChannelMode mode() const { return m_mode; }

    qint64 write(const QByteArray& data);

signals:
    void bytesReceived(const QByteArray& data);
    void stateChanged(bool open);
    void errorOccurred(const QString& message);

private:
    void teardown();
    void setupChannel(AbstractChannel* channel, ChannelMode mode);

    AbstractChannel* m_channel = nullptr;
    QThread* m_thread = nullptr;
    ChannelMode m_mode = ChannelMode::Serial;
    bool m_open = false;
};

} // namespace comm
