#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <QString>
#include <QUrl>

// Connection parameters for a TVHeadend server. The password only ever lives
// in memory; persistence goes through the system secret store (AppSettings).
struct ServerConfig {
    QString host;
    int port = 9981;
    bool https = false;
    QString username;
    QString password;

    bool isValid() const;
    // Base URL including an optional web root, always ending in '/'.
    QUrl baseUrl() const;
    // Resolves a server relative path (e.g. "api/serverinfo") against baseUrl().
    QUrl url(const QString &path) const;
    // Identifies the server (not the user) so caches of another server can be discarded.
    QString identity() const;

    bool operator==(const ServerConfig &other) const = default;
};

#endif // SERVERCONFIG_H
