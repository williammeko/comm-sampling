#pragma once

#include <QByteArray>
#include <QObject>
#include <QThread>

namespace comm {

// Base class for all I/O channels (serial, TCP client, TCP server).
// A channel lives in its own QThread; open()/close() run there, and
// bytesReceived/opened/closed/errorOccurred are delivered to the GUI thread
// through queued signal/slot connections.
class AbstractChannel : public QObject
{
    Q_OBJECT

public:
    explicit AbstractChannel(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    ~AbstractChannel() override = default;

    Q_INVOKABLE virtual bool open() = 0;
    Q_INVOKABLE virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // Thread-safe write: may be called from any thread.
    qint64 write(const QByteArray& data)
    {
        if (QThread::currentThread() == thread())
            return doWrite(data);

        qint64 result = -1;
        QMetaObject::invokeMethod(
            this,
            [this, &result, data]() { result = doWrite(data); },
            Qt::BlockingQueuedConnection);
        return result;
    }

signals:
    void bytesReceived(const QByteArray& data);
    void errorOccurred(const QString& message);
    void opened();
    void closed();

protected:
    virtual qint64 doWrite(const QByteArray& data) = 0;
};

} // namespace comm
