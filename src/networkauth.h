/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef NETWORKAUTH_H
#define NETWORKAUTH_H

#include "serverconfig.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace NetworkAuth
{
// Thread-safe snapshot of the active server credentials. Used by network
// access managers living on other threads (e.g. the QML image loader).
void setActiveServer(const ServerConfig &config);
ServerConfig activeServer();

// Answers HTTP Basic/Digest challenges from TVHeadend. Credentials can be
// overridden per reply with setReplyCredentials(); otherwise the active
// server credentials are used. Each reply gets exactly one attempt so wrong
// credentials fail fast instead of looping.
void install(QNetworkAccessManager *networkManager);
void setReplyCredentials(QNetworkReply *reply, const QString &user, const QString &password);
}

#endif // NETWORKAUTH_H
