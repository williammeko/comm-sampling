#pragma once

#include <QByteArray>
#include <QObject>
#include <QStringList>
#include <QTimer>

#include <functional>

namespace comm {

// Handles one-shot and periodic (loop) sending, plus the recent-send history.
class DataSender : public QObject
{
    Q_OBJECT

public:
    using WriteFunc = std::function<qint64(const QByteArray&)>;

    explicit DataSender(QObject* parent = nullptr);

    void setWriteFunc(WriteFunc func);

    void sendOnce(const QByteArray& data);
    void startLoop(const QByteArray& data, int intervalMs);
    void stopLoop();
    bool isLooping() const { return m_timer.isActive(); }

    // Newest first, deduplicated, capped at 50 entries.
    void addToHistory(const QString& hexText);
    const QStringList& history() const { return m_history; }

    void setHistory(const QStringList& history)
    {
        m_history = history;
        emit historyChanged();
    }

signals:
    void historyChanged();

private:
    QTimer m_timer;
    QByteArray m_loopData;
    WriteFunc m_write;
    QStringList m_history;
};

} // namespace comm
