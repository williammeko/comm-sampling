#include "ChannelManager.h"

#include "communication/SerialChannel.h"
#include "communication/TcpClientChannel.h"
#include "communication/TcpServerChannel.h"

namespace comm {

ChannelManager::ChannelManager(QObject* parent)
    : QObject(parent)
{
}

ChannelManager::~ChannelManager()
{
    teardown();
}

void ChannelManager::startSerial(const SerialConfig& cfg)
{
    setupChannel(new SerialChannel(cfg), ChannelMode::Serial);
}

void ChannelManager::startTcpClient(const TcpClientConfig& cfg)
{
    setupChannel(new TcpClientChannel(cfg), ChannelMode::TcpClient);
}

void ChannelManager::startTcpServer(const TcpServerConfig& cfg)
{
    setupChannel(new TcpServerChannel(cfg), ChannelMode::TcpServer);
}

void ChannelManager::setupChannel(AbstractChannel* channel, ChannelMode mode)
{
    teardown();

    m_channel = channel;
    m_mode = mode;

    m_thread = new QThread(this);
    m_channel->moveToThread(m_thread);

    connect(m_channel, &AbstractChannel::bytesReceived, this, &ChannelManager::bytesReceived);
    connect(m_channel, &AbstractChannel::opened, this, [this]() {
        m_open = true;
        emit stateChanged(true);
    });
    connect(m_channel, &AbstractChannel::closed, this, [this]() {
        m_open = false;
        emit stateChanged(false);
    });
    connect(m_channel, &AbstractChannel::errorOccurred, this, [this](const QString& msg) {
        emit errorOccurred(msg);
    });

    // open() runs inside the new I/O thread.
    connect(m_thread, &QThread::started, m_channel, [this]() {
        if (!m_channel->open())
            emit m_channel->errorOccurred(QStringLiteral("Failed to open channel"));
    });

    m_thread->start();
}

void ChannelManager::teardown()
{
    if (m_channel && m_thread) {
        if (m_thread->isRunning()) {
            QMetaObject::invokeMethod(m_channel, "close", Qt::BlockingQueuedConnection);
            m_thread->quit();
            m_thread->wait();
        }
        delete m_channel;
        m_channel = nullptr;
    }

    if (m_thread) {
        delete m_thread;
        m_thread = nullptr;
    }

    m_open = false;
}

void ChannelManager::stop()
{
    teardown();
    emit stateChanged(false);
}

qint64 ChannelManager::write(const QByteArray& data)
{
    if (!m_open || !m_channel)
        return -1;
    return m_channel->write(data);
}

} // namespace comm
