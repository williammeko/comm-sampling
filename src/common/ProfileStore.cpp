#include "ProfileStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
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

bool readAll(QJsonObject& profiles, QStringList& order)
{
    QFile file(ProfileStore::filePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    const QJsonObject root = doc.object();
    profiles = root.value(QStringLiteral("profiles")).toObject();

    order.clear();
    const QJsonArray orderArr = root.value(QStringLiteral("order")).toArray();
    for (const auto& value : orderArr)
        order.append(value.toString());
    return true;
}

bool writeAll(const QJsonObject& profiles, const QStringList& order)
{
    QJsonObject root;
    root.insert(QStringLiteral("profiles"), profiles);

    QJsonArray orderArr;
    for (const QString& name : order)
        orderArr.append(name);
    root.insert(QStringLiteral("order"), orderArr);

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
    QStringList order;
    readAll(profiles, order);

    QStringList all = profiles.keys();
    all.removeAll(RecentName);

    // Most recently saved first, then the rest sorted alphabetically.
    QStringList result;
    for (const QString& name : order) {
        if (all.removeAll(name) > 0)
            result.append(name);
    }
    all.sort(Qt::CaseInsensitive);
    result.append(all);
    return result;
}

bool ProfileStore::load(const QString& name, QJsonObject& out)
{
    QJsonObject profiles;
    QStringList order;
    if (!readAll(profiles, order))
        return false;
    if (!profiles.contains(name))
        return false;
    out = profiles.value(name).toObject();
    return true;
}

bool ProfileStore::save(const QString& name, const QJsonObject& data)
{
    QJsonObject profiles;
    QStringList order;
    readAll(profiles, order); // ignore failure; start fresh if missing
    profiles.insert(name, data);

    order.removeAll(name);
    order.prepend(name);

    return writeAll(profiles, order);
}

bool ProfileStore::remove(const QString& name)
{
    QJsonObject profiles;
    QStringList order;
    if (!readAll(profiles, order))
        return false;
    if (!profiles.contains(name))
        return false;

    profiles.remove(name);
    order.removeAll(name);
    return writeAll(profiles, order);
}

} // namespace comm
