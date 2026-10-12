#include "serverconfig.h"

bool ServerConfig::isValid() const
{
    return baseUrl().isValid() && !host.trimmed().isEmpty();
}

QUrl ServerConfig::baseUrl() const
{
    QString hostPart = host.trimmed();
    bool useTls = https;
    int effectivePort = port;

    // Be forgiving: accept "http://host:port/webroot" style input in the host field.
    if (hostPart.startsWith(QLatin1String("https://"), Qt::CaseInsensitive)) {
        useTls = true;
        hostPart = hostPart.mid(8);
    } else if (hostPart.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)) {
        hostPart = hostPart.mid(7);
    }

    QString path;
    const int slash = hostPart.indexOf(QLatin1Char('/'));
    if (slash >= 0) {
        path = hostPart.mid(slash);
        hostPart = hostPart.left(slash);
    }

    while (path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }

    if (hostPart.startsWith(QLatin1Char('['))) {
        // IPv6 literal, optionally followed by ":port"
        const int close = hostPart.indexOf(QLatin1Char(']'));
        if (close > 0 && hostPart.size() > close + 2
            && hostPart.at(close + 1) == QLatin1Char(':')) {
            bool ok = false;
            const int explicitPort = hostPart.mid(close + 2).toInt(&ok);
            if (ok) {
                effectivePort = explicitPort;
            }

            hostPart = hostPart.left(close + 1);
        }
    } else if (hostPart.count(QLatin1Char(':')) == 1) {
        bool ok = false;
        const int explicitPort = hostPart.section(QLatin1Char(':'), 1).toInt(&ok);
        if (ok) {
            effectivePort = explicitPort;
        }

        hostPart = hostPart.section(QLatin1Char(':'), 0, 0);
    }

    if (hostPart.isEmpty()) {
        return {};
    }

    QUrl url;
    url.setScheme(useTls ? QStringLiteral("https") : QStringLiteral("http"));
    url.setHost(hostPart);

    if (effectivePort > 0) {
        url.setPort(effectivePort);
    }

    url.setPath(path + QLatin1Char('/'));
    return url;
}

QUrl ServerConfig::url(const QString &path) const
{
    QString relative = path;
    while (relative.startsWith(QLatin1Char('/'))) {
        relative.remove(0, 1);
    }

    return baseUrl().resolved(QUrl(relative));
}

QString ServerConfig::identity() const
{
    return baseUrl().toString(QUrl::RemoveUserInfo);
}
