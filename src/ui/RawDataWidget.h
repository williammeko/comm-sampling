#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QWidget>

class QPlainTextEdit;
class QSpinBox;

namespace comm {

// Shows raw bytes received from the device as uppercase space-separated hex.
// Bytes received >=5 ms after the previous chunk start a new line. Only the
// most recent N bytes (user-configurable) are kept. Auto-scrolls to bottom.
class RawDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RawDataWidget(QWidget* parent = nullptr);

    int maxBytes() const;
    void setMaxBytes(int bytes);

    void appendBytes(const QByteArray& bytes);
    void clear();

signals:
    void settingsChanged();

private:
    void refreshDisplay();

    QList<QByteArray> m_lines;
    int m_totalBytes = 0;
    QElapsedTimer m_clock;
    qint64 m_lastAppendMs = -1;

    QSpinBox* m_maxBytesSpin = nullptr;
    QPlainTextEdit* m_view = nullptr;
};

} // namespace comm
