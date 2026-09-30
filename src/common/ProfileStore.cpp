#include "ProfileStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStandardPaths>

namespace comm {

const QString ProfileStore::RecentName = QStringLiteral("recent");

QString ProfileStore::filePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QStringLiteral("/comm-sampling-profiles.txt");
}

namespace {

bool readAll(QJsonObject& profiles)
{
    QFile file(ProfileStore::filePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    profiles = doc.object().value(QStringLiteral("profiles")).toObject();
    return true;
}

bool writeAll(const QJsonObject& profiles)
{
    QJsonObject root;
    root.insert(QStringLiteral("profiles"), profiles);

    const QString path = ProfileStore::filePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace

QStringList ProfileStore::names()
{
    QJsonObject profiles;
    readAll(profiles);

    QStringList result = profiles.keys();
    result.removeAll(RecentName);
    result.sort(Qt::CaseInsensitive);
    return result;
}

bool ProfileStore::load(const QString& name, QJsonObject& out)
{
    QJsonObject profiles;
    if (!readAll(profiles))
        return false;
    if (!profiles.contains(name))
        return false;
    out = profiles.value(name).toObject();
    return true;
}

bool ProfileStore::save(const QString& name, const QJsonObject& data)
{
    QJsonObject profiles;
    readAll(profiles); // ignore failure; start fresh if missing
    profiles.insert(name, data);
    return writeAll(profiles);
}

bool ProfileStore::remove(const QString& name)
{
    QJsonObject profiles;
    if (!readAll(profiles))
        return false;
    if (!profiles.contains(name))
        return false;
    profiles.remove(name);
    return writeAll(profiles);
}

} // namespace comm
