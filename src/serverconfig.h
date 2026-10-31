/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <QString>
#include <QUrl>

// Connection parameters for a TVHeadend server. The password only ever lives
// in memory; persistence goes through the system secret store (AppSettings).
struct ServerConfig {
    QString host;
    QString username;
    QString password;

    int port = 9981;
    bool https = false;

    [[nodiscard]] bool isValid() const;

    // Base URL including an optional web root, always ending in '/'.
    [[nodiscard]] QUrl baseUrl() const;

    // Resolves a server relative path (e.g. "api/serverinfo") against baseUrl().
    [[nodiscard]] QUrl url(const QString &path) const;

    // Identifies the server (not the user) so caches of another server can be discarded.
    [[nodiscard]] QString identity() const;

    bool operator==(const ServerConfig &other) const = default;
};

#endif // SERVERCONFIG_H
