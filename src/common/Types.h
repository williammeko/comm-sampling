#pragma once

#include <QString>
#include <QHostAddress>
#include <QColor>

namespace comm {

// How the app connects to a device.
enum class ChannelMode {
    Serial,
    TcpClient,
    TcpServer,
};

// Serial-port settings. Values for data bits intentionally mirror
// QSerialPort::DataBits (5..8); parity/stop-bits are mapped explicitly.
enum class DataBits {
    Bits5 = 5,
    Bits6 = 6,
    Bits7 = 7,
    Bits8 = 8,
};

enum class Parity {
    None,
    Even,
    Odd,
    Space,
    Mark,
};

enum class StopBits {
    One,
    OneAndHalf,
    Two,
};

struct SerialConfig {
    QString portName;
    int baudRate = 115200;
    DataBits dataBits = DataBits::Bits8;
    Parity parity = Parity::None;
    StopBits stopBits = StopBits::One;
};

struct TcpClientConfig {
    QString host;
    quint16 port = 0;
};

struct TcpServerConfig {
    QHostAddress listenAddress = QHostAddress::Any;
    quint16 port = 0;
};

// Matcher conversion settings.
enum class NumberType {
    UInt,
    Int,
    Ieee754,
};

enum class ByteOrder {
    BigEndian,
    LittleEndian,
};

// A named, colored chart series (one decimal picker).
struct SeriesInfo {
    QString name;
    QColor color;
};

} // namespace comm
