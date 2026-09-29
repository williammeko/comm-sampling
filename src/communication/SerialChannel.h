#pragma once

#include "communication/AbstractChannel.h"
#include "common/Types.h"

#include <QSerialPort>

namespace comm {

class SerialChannel : public AbstractChannel
{
    Q_OBJECT

public:
    explicit SerialChannel(const SerialConfig& cfg, QObject* parent = nullptr);
    ~SerialChannel() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;

protected:
    qint64 doWrite(const QByteArray& data) override;

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError error);

private:
    QSerialPort* m_port = nullptr;
    SerialConfig m_cfg;
};

} // namespace comm
