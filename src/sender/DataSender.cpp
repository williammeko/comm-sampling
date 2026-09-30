#include "DataSender.h"

#include <QtGlobal>

namespace comm {

DataSender::DataSender(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        if (m_write && m_loopProducer)
            m_write(m_loopProducer());
    });
}

void DataSender::setWriteFunc(WriteFunc func)
{
    m_write = std::move(func);
}

void DataSender::sendOnce(const QByteArray& data)
{
    if (m_write)
        m_write(data);
}

void DataSender::startLoop(const DataProducer& producer, int intervalMs)
{
    stopLoop();
    m_loopProducer = producer;
    m_timer.start(qMax(1, intervalMs));
}

void DataSender::stopLoop()
{
    m_timer.stop();
}

void DataSender::addToHistory(const QString& hexText)
{
    if (hexText.isEmpty())
        return;

    m_history.removeAll(hexText); // dedupe: remove any existing copy
    m_history.prepend(hexText);   // newest first
    while (m_history.size() > 50)
        m_history.removeLast();

    emit historyChanged();
}

} // namespace comm
