#pragma once

#include <QString>
#include <QStringList>

#include "common/Types.h"

namespace comm {

// Persists recently-used settings and histories to a JSON file in the
// application data folder (e.g. ~/Library/Application Support/comm-sampling
// on macOS).
class RecentDataStore
{
public:
    struct State
    {
        ChannelMode mode = ChannelMode::Serial;

        QString serialPortName;
        int baudRate = 115200;
        int dataBits = 8;
        int parity = 0;
        int stopBits = 0;

        QString clientHost;
        int clientPort = 8080;

        QString serverInterface = QStringLiteral("0.0.0.0");
        int serverPort = 8080;

        int byteCacheSize = 10000;
        int rawDataBytes = 2000;
        int loopIntervalMs = 1000;

        int sendStrategy = 0;
        int sendCrcAlgorithm = 0;
        QString fieldSpec;
        QStringList fieldSpecHistory;

        QStringList sendHistory;
        QStringList keywordHistory;
    };

    static QString filePath();
    static bool load(State& out);
    static bool save(const State& in);
};

} // namespace comm
