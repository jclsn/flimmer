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
