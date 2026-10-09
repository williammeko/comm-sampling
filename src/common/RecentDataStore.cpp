#include "RecentDataStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace comm {

QString RecentDataStore::filePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QStringLiteral("/comm-sampling-recent.txt");
}

bool RecentDataStore::load(State& out)
{
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    const QJsonObject root = doc.object();
    const QJsonObject comm = root.value(QStringLiteral("comm")).toObject();

    out.mode = static_cast<ChannelMode>(
        comm.value(QStringLiteral("mode")).toInt(static_cast<int>(ChannelMode::Serial)));
    out.serialPortName = comm.value(QStringLiteral("serialPort")).toString();
    out.baudRate = comm.value(QStringLiteral("baud")).toInt(115200);
    out.dataBits = comm.value(QStringLiteral("dataBits")).toInt(8);
    out.parity = comm.value(QStringLiteral("parity")).toInt(0);
    out.stopBits = comm.value(QStringLiteral("stopBits")).toInt(0);
    out.clientHost = comm.value(QStringLiteral("clientHost")).toString();
    out.clientPort = comm.value(QStringLiteral("clientPort")).toInt(8080);
    out.serverInterface = comm.value(QStringLiteral("serverInterface"))
                              .toString(QStringLiteral("0.0.0.0"));
    out.serverPort = comm.value(QStringLiteral("serverPort")).toInt(8080);
    out.byteCacheSize = comm.value(QStringLiteral("byteCacheSize")).toInt(10000);
    out.rawDataBytes = comm.value(QStringLiteral("rawDataBytes")).toInt(2000);
    out.loopIntervalMs = comm.value(QStringLiteral("loopIntervalMs")).toInt(1000);
    out.sendStrategy = root.value(QStringLiteral("sendStrategy")).toInt(0);
    out.sendCrcAlgorithm = root.value(QStringLiteral("sendCrcAlgorithm")).toInt(0);
    out.fieldSpec = root.value(QStringLiteral("fieldSpec")).toString();
    out.templateData = root.value(QStringLiteral("templateData")).toString();
    out.showValues = root.value(QStringLiteral("showValues")).toBool(true);

    out.fieldSpecHistory.clear();
    const QJsonArray fieldSpecArr = root.value(QStringLiteral("fieldSpecHistory")).toArray();
    for (const auto& value : fieldSpecArr)
        out.fieldSpecHistory.append(value.toString());

    out.sendHistory.clear();
    const QJsonArray sendArr = root.value(QStringLiteral("sendHistory")).toArray();
    for (const auto& value : sendArr)
        out.sendHistory.append(value.toString());

    out.keywordHistory.clear();
    const QJsonArray keywordArr = root.value(QStringLiteral("keywordHistory")).toArray();
    for (const auto& value : keywordArr)
        out.keywordHistory.append(value.toString());

    return true;
}

bool RecentDataStore::save(const State& in)
{
    QJsonObject comm;
    comm.insert(QStringLiteral("mode"), static_cast<int>(in.mode));
    comm.insert(QStringLiteral("serialPort"), in.serialPortName);
    comm.insert(QStringLiteral("baud"), in.baudRate);
    comm.insert(QStringLiteral("dataBits"), in.dataBits);
    comm.insert(QStringLiteral("parity"), in.parity);
    comm.insert(QStringLiteral("stopBits"), in.stopBits);
    comm.insert(QStringLiteral("clientHost"), in.clientHost);
    comm.insert(QStringLiteral("clientPort"), in.clientPort);
    comm.insert(QStringLiteral("serverInterface"), in.serverInterface);
    comm.insert(QStringLiteral("serverPort"), in.serverPort);
    comm.insert(QStringLiteral("byteCacheSize"), in.byteCacheSize);
    comm.insert(QStringLiteral("rawDataBytes"), in.rawDataBytes);
    comm.insert(QStringLiteral("loopIntervalMs"), in.loopIntervalMs);

    QJsonArray sendArr;
    for (const QString& s : in.sendHistory)
        sendArr.append(s);

    QJsonArray keywordArr;
    for (const QString& s : in.keywordHistory)
        keywordArr.append(s);

    QJsonArray fieldSpecArr;
    for (const QString& s : in.fieldSpecHistory)
        fieldSpecArr.append(s);

    QJsonObject root;
    root.insert(QStringLiteral("comm"), comm);
    root.insert(QStringLiteral("sendStrategy"), in.sendStrategy);
    root.insert(QStringLiteral("sendCrcAlgorithm"), in.sendCrcAlgorithm);
    root.insert(QStringLiteral("fieldSpec"), in.fieldSpec);
    root.insert(QStringLiteral("fieldSpecHistory"), fieldSpecArr);
    root.insert(QStringLiteral("templateData"), in.templateData);
    root.insert(QStringLiteral("showValues"), in.showValues);
    root.insert(QStringLiteral("sendHistory"), sendArr);
    root.insert(QStringLiteral("keywordHistory"), keywordArr);

    const QString path = filePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace comm
