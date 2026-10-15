// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "networkauth.h"

#include <QAuthenticator>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace
{
QMutex s_mutex;
ServerConfig s_active;

constexpr const char *s_attempts = "tvhAuthAttempts";
constexpr const char *s_user = "tvhUser";
constexpr const char *s_password = "tvhPassword";
}

void NetworkAuth::setActiveServer(const ServerConfig &config)
{
    QMutexLocker lock(&s_mutex);
    s_active = config;
}

ServerConfig NetworkAuth::activeServer()
{
    QMutexLocker lock(&s_mutex);
    return s_active;
}

void NetworkAuth::setReplyCredentials(QNetworkReply *reply,
                                      const QString &user,
                                      const QString &password)
{
    reply->setProperty(s_user, user);
    reply->setProperty(s_password, password);
}

void NetworkAuth::install(QNetworkAccessManager *networkManager)
{
    QObject::connect(networkManager,
                     &QNetworkAccessManager::authenticationRequired,
                     networkManager,
                     [](QNetworkReply *reply, QAuthenticator *auth) {
                         const int attempts = reply->property(s_attempts).toInt();
                         if (attempts > 0) {
                             return; // give up, the reply finishes with an authentication error
                         }

                         reply->setProperty(s_attempts, attempts + 1);

                         QString user;
                         QString password;
                         if (reply->property(s_user).isValid()) {
                             user = reply->property(s_user).toString();
                             password = reply->property(s_password).toString();
                         } else {
                             const ServerConfig active = activeServer();
                             // Never leak credentials to a host other than the configured server.
                             if (reply->url().host() != active.baseUrl().host()) {
                                 return;
                             }

                             user = active.username;
                             password = active.password;
                         }

                         if (user.isEmpty()) {
                             return;
                         }

                         auth->setUser(user);
                         auth->setPassword(password);
                     });
}
