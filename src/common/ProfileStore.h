#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace comm {

// Stores named profiles (full app-state snapshots) in comm-sampling-profiles.txt.
// Includes a special "recent" profile that is auto-saved and auto-loaded.
class ProfileStore
{
public:
    static const QString RecentName;

    static QString filePath();
    static QStringList names(); // all profile names except "recent"

    static bool load(const QString& name, QJsonObject& out);
    static bool save(const QString& name, const QJsonObject& data);
    static bool remove(const QString& name);
};

} // namespace comm
