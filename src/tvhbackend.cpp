// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "tvhbackend.h"

#include "appsettings.h"
#include "channelmodel.h"
#include "tvhapi.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QQmlEngine>

#include <algorithm>

namespace
{
constexpr int s_epgPageSize = 2000;
constexpr int s_maxEpgEvents = 200000;
constexpr int s_tickMs = 15 * 1000;
constexpr int s_epgRefreshMs = 30 * 60 * 1000;
constexpr int s_retryMs = 60 * 1000;
// Longest pause; the recording stops on its own after that.
constexpr int s_timeshiftSecs = 4 * 3600;
// Marks the recordings made for pausing, so leftovers can be found and removed.
const QLatin1String s_timeshiftComment("plasma-tv-timeshift");

ServerConfig configFromValues(const QVariantMap &values, const ServerConfig &fallback)
{
    ServerConfig config = fallback;
    config.host = values.value(QStringLiteral("host"), config.host).toString().trimmed();
    config.port = values.value(QStringLiteral("port"), config.port).toInt();
    config.https = values.value(QStringLiteral("useHttps"), config.https).toBool();
    config.username =
        values.value(QStringLiteral("username"), config.username).toString().trimmed();

    if (values.contains(QStringLiteral("password"))) {
        config.password = values.value(QStringLiteral("password")).toString();
    }

    return config;
}

QJsonArray entriesOf(const QJsonDocument &document)
{
    return document.object().value(QLatin1String("entries")).toArray();
}

QString firstUuid(const QJsonDocument &document)
{
    const QJsonValue uuid = document.object().value(QLatin1String("uuid"));
    if (uuid.isArray()) {
        return uuid.toArray().first().toString();
    }

    return uuid.toString();
}
}

TvhBackend *TvhBackend::instance()
{
    static TvhBackend *s_instance = new TvhBackend;
    return s_instance;
}

TvhBackend *TvhBackend::create(QQmlEngine *, QJSEngine *)
{
    auto *self = instance();
    QQmlEngine::setObjectOwnership(self, QQmlEngine::CppOwnership);
    return self;
}

TvhBackend::TvhBackend(QObject *parent)
    : QObject(parent)
    , m_api(new TvhApi(this))
    , m_channels(new ChannelModel(&m_store, this))
{
    m_database.open();

    m_tickTimer.setInterval(s_tickMs);
    connect(&m_tickTimer, &QTimer::timeout, this, [this]() {
        m_channels->refreshNow();
        Q_EMIT tick();
    });
    m_tickTimer.start();

    m_epgTimer.setInterval(s_epgRefreshMs);
    connect(&m_epgTimer, &QTimer::timeout, this, &TvhBackend::refresh);

    m_retryTimer.setInterval(s_retryMs);
    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout, this, &TvhBackend::refresh);

    AppSettings *settings = AppSettings::instance();
    connect(settings, &AppSettings::saved, this, &TvhBackend::applySettings);
    connect(settings, &AppSettings::credentialsReadyChanged, this, [this]() {
        applySettings(true);
    });

    m_api->setServer(settings->server());
    m_channels->setBaseUrl(settings->server().baseUrl());
    loadCache();

    if (!settings->configured()) {
        setStatus(Unconfigured, tr("Not configured"));
    } else if (settings->credentialsReady()) {
        refresh();
    } else {
        setStatus(Connecting, tr("Unlocking credentials…"));
    }
}

void TvhBackend::applySettings(bool serverChanged)
{
    const ServerConfig server = AppSettings::instance()->server();
    if (serverChanged) {
        m_api->setServer(server);
        m_channels->setBaseUrl(server.baseUrl());

        if (m_database.serverIdentity() != server.identity()) {
            clearCache();
        }
    }

    refresh();
}

void TvhBackend::loadCache()
{
    const ServerConfig server = AppSettings::instance()->server();
    if (!server.isValid() || m_database.serverIdentity() != server.identity()) {
        return;
    }

    m_store.setEvents(m_database.loadEvents(QDateTime::currentSecsSinceEpoch()));
    m_channels->setChannels(m_database.loadChannels());
    const qint64 last = m_database.lastUpdate();
    m_lastUpdate = last > 0 ? QDateTime::fromSecsSinceEpoch(last) : QDateTime();
    ++m_epgRevision;
    Q_EMIT epgChanged();
}

void TvhBackend::clearCache()
{
    m_database.clear();
    m_store.clear();
    m_channels->setChannels({});
    m_lastUpdate = {};
    ++m_epgRevision;
    Q_EMIT epgChanged();
}

void TvhBackend::refresh()
{
    AppSettings *settings = AppSettings::instance();
    m_retryTimer.stop();

    if (!settings->configured()) {
        setStatus(Unconfigured, tr("Not configured"));
        return;
    }

    if (!settings->credentialsReady()) {
        return;
    }

    const int generation = ++m_generation;
    m_api->abortAll();
    m_api->setServer(settings->server());
    setBusy(true);

    if (m_status != Online) {
        setStatus(Connecting, tr("Connecting to %1…").arg(settings->server().baseUrl().host()));
    }

    m_api->get(QStringLiteral("api/serverinfo"), {}, [this, generation](const ApiResult &result) {
        if (generation != m_generation) {
            return;
        }

        if (!result.ok) {
            setBusy(false);
            setStatus(Offline, result.error);
            m_retryTimer.start();
            return;
        }

        const QJsonObject info = result.json().object();
        m_serverName = info.value(QLatin1String("name")).toString(QStringLiteral("Tvheadend"));
        m_serverVersion = info.value(QLatin1String("sw_version")).toString();
        Q_EMIT serverInfoChanged();
        fetchChannels(generation);
    });
}

void TvhBackend::fetchChannels(int generation)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("start"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("100000"));
    m_api->get(QStringLiteral("api/channel/grid"),
               query,
               [this, generation](const ApiResult &result) {
                   if (generation != m_generation) {
                       return;
                   }

                   if (!result.ok) {
                       setBusy(false);
                       setStatus(Offline, tr("Loading channels failed: %1").arg(result.error));
                       m_retryTimer.start();
                       return;
                   }

                   QVector<Channel> channels;
                   for (const QJsonValue &value : entriesOf(result.json())) {
                       const QJsonObject object = value.toObject();
                       if (!object.value(QLatin1String("enabled")).toBool(true)) {
                           continue;
                       }

                       channels.append(Channel::fromJson(object));
                   }

                   std::stable_sort(channels.begin(),
                                    channels.end(),
                                    [](const Channel &a, const Channel &b) {
                                        // Unnumbered channels (0) go to the end, alphabetically.
                                        if ((a.number == 0) != (b.number == 0)) {
                                            return a.number != 0;
                                        }

                                        if (a.number != b.number) {
                                            return a.number < b.number;
                                        }

                                        return a.name.localeAwareCompare(b.name) < 0;
                                    });

                   m_database.setServerIdentity(AppSettings::instance()->server().identity());
                   m_database.saveChannels(channels);
                   m_channels->setChannels(channels);
                   setStatus(Online, tr("Connected"));
                   m_epgTimer.start();
                   removeStaleTimeshifts();

                   m_incoming.clear();
                   fetchEpgPage(generation, 0);
               });
}

void TvhBackend::fetchEpgPage(int generation, int offset)
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 end = now + qint64(AppSettings::instance()->guideDays()) * 24 * 3600;
    const QJsonArray filter{
        QJsonObject{{QStringLiteral("field"), QStringLiteral("stop")},
                    {QStringLiteral("type"), QStringLiteral("numeric")},
                    {QStringLiteral("comparison"), QStringLiteral("gt")},
                    {QStringLiteral("value"), now}},
        QJsonObject{{QStringLiteral("field"), QStringLiteral("start")},
                    {QStringLiteral("type"), QStringLiteral("numeric")},
                    {QStringLiteral("comparison"), QStringLiteral("lt")},
                    {QStringLiteral("value"), end}},
    };

    QUrlQuery form;
    form.addQueryItem(QStringLiteral("start"), QString::number(offset));
    form.addQueryItem(QStringLiteral("limit"), QString::number(s_epgPageSize));
    form.addQueryItem(QStringLiteral("sort"), QStringLiteral("start"));
    form.addQueryItem(QStringLiteral("dir"), QStringLiteral("ASC"));
    form.addQueryItem(QStringLiteral("filter"),
                      QString::fromUtf8(QJsonDocument(filter).toJson(QJsonDocument::Compact)));

    m_api->post(
        QStringLiteral("api/epg/events/grid"),
        form,
        [this, generation, offset, now, end](const ApiResult &result) {
            if (generation != m_generation) {
                return;
            }

            if (!result.ok) {
                setBusy(false);
                Q_EMIT notification(tr("Loading the TV guide failed: %1").arg(result.error));
                return;
            }

            const QJsonDocument document = result.json();
            const QJsonArray entries = entriesOf(document);
            for (const QJsonValue &value : entries) {
                EpgEvent event = EpgEvent::fromJson(value.toObject());
                // Filter client side as well in case the server ignored the filter.
                if (event.stop > now && event.start < end) {
                    m_incoming.append(std::move(event));
                }
            }

            const int total = document.object().value(QLatin1String("totalCount")).toInt(-1);
            const int fetched = offset + int(entries.size());
            const bool more = entries.size() == s_epgPageSize && (total < 0 || fetched < total)
                && fetched < s_maxEpgEvents;
            if (more) {
                fetchEpgPage(generation, fetched);
            } else {
                finishEpg(generation);
            }
        });
}

void TvhBackend::finishEpg(int generation)
{
    if (generation != m_generation) {
        return;
    }

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    m_database.saveEvents(m_incoming, now);
    m_store.setEvents(std::move(m_incoming));
    m_incoming = {};
    m_lastUpdate = QDateTime::fromSecsSinceEpoch(now);
    m_channels->refreshNow();
    ++m_epgRevision;
    setBusy(false);
    Q_EMIT epgChanged();
}

void TvhBackend::testConnection(const QVariantMap &values)
{
    const ServerConfig config = configFromValues(values, AppSettings::instance()->server());
    if (!config.isValid()) {
        Q_EMIT connectionTestFinished(false, tr("Please enter a server address"));
        return;
    }

    m_api->probe(config, QStringLiteral("api/serverinfo"), [this](const ApiResult &result) {
        if (!result.ok) {
            Q_EMIT connectionTestFinished(false, result.error);
            return;
        }

        const QJsonObject info = result.json().object();
        if (!info.contains(QLatin1String("api_version"))) {
            Q_EMIT connectionTestFinished(
                false,
                tr("The server answered, but does not look like TVHeadend"));
            return;
        }

        Q_EMIT connectionTestFinished(
            true,
            tr("Connected to %1 %2")
                .arg(info.value(QLatin1String("name")).toString(QStringLiteral("Tvheadend")),
                     info.value(QLatin1String("sw_version")).toString()));
    });
}

QVariantList TvhBackend::eventsForChannel(const QString &uuid, double fromMs, double toMs) const
{
    QVariantList result;
    const auto events = m_store.range(uuid, qint64(fromMs / 1000.0), qint64(toMs / 1000.0));
    result.reserve(events.size());

    for (const EpgEvent *event : events) {
        result.append(event->toVariant());
    }

    return result;
}

QVariantMap TvhBackend::eventAt(const QString &uuid, double timeMs) const
{
    const EpgEvent *event = m_store.eventAt(uuid, qint64(timeMs / 1000.0));
    return event ? event->toVariant() : QVariantMap();
}

QVariantMap TvhBackend::nextEvent(const QString &uuid, double afterMs) const
{
    const EpgEvent *event = m_store.nextAfter(uuid, qint64(afterMs / 1000.0));
    return event ? event->toVariant() : QVariantMap();
}

QVariantMap TvhBackend::eventById(double id) const
{
    const EpgEvent *event = m_store.byId(qint64(id));
    return event ? event->toVariant() : QVariantMap();
}

void TvhBackend::requestStream(const QString &uuid)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("profile"), AppSettings::instance()->streamProfile());
    requestPlaylist(uuid,
                    QStringLiteral("channel/") + uuid,
                    QStringLiteral("stream/channel/") + uuid,
                    query);
}

void TvhBackend::requestRecording(const QString &dvrUuid)
{
    // The playlist of all recordings is the one that hands out tickets for them, recordings
    // still in progress included.
    const QString path = QStringLiteral("dvrfile/") + dvrUuid;
    requestPlaylist(dvrUuid, QStringLiteral("recordings"), path, {}, QLatin1Char('/') + path);
}

void TvhBackend::requestPlaylist(const QString &key,
                                 const QString &path,
                                 const QString &directPath,
                                 const QUrlQuery &query,
                                 const QString &entryPath)
{
    const ServerConfig server = m_api->server();
    QUrl direct = server.url(directPath);
    direct.setQuery(query);

    if (server.username.isEmpty()) {
        Q_EMIT streamReady(key, direct);
        return;
    }

    // Prefer a short-lived ticket so the password never ends up in the media URL.
    m_api->get(QStringLiteral("playlist/ticket/") + path,
               query,
               [this, key, server, direct, query, entryPath](const ApiResult &result) {
                   if (result.ok) {
                       const QStringList lines =
                           QString::fromUtf8(result.body).split(QLatin1Char('\n'));
                       for (const QString &line : lines) {
                           const QUrl ticketUrl(line.trimmed());

                           if (!ticketUrl.isValid()
                               || !ticketUrl.scheme().startsWith(QLatin1String("http"))) {
                               continue;
                           }

                           // Rebuild against our own base URL: the server may advertise an address
                           // we can't reach.
                           QUrl url = server.baseUrl();
                           url.setPath(ticketUrl.path());
                           QUrlQuery ticketQuery(ticketUrl);

                           for (const auto &[name, value] : query.queryItems()) {
                               if (!ticketQuery.hasQueryItem(name)) {
                                   ticketQuery.addQueryItem(name, value);
                               }
                           }

                           url.setQuery(ticketQuery);
                           Q_EMIT streamReady(key, url);
                           return;
                       }
                   } else if (result.httpStatus == 401 || result.httpStatus == 403) {
                       Q_EMIT streamFailed(key, result.error);
                       return;
                   }

                   // Older servers without ticket support: fall back to credentials in the URL.
                   QUrl url = direct;
                   url.setUserName(server.username);
                   url.setPassword(server.password);
                   Q_EMIT streamReady(key, url);
               });
}

void TvhBackend::startTimeshift(const QString &channelUuid)
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const QJsonObject title{{QStringLiteral("eng"), tr("Paused in Plasma TV")}};
    const QJsonObject conf{
        {QStringLiteral("channel"), channelUuid},
        {QStringLiteral("start"), now},
        {QStringLiteral("stop"), now + s_timeshiftSecs},
        {QStringLiteral("title"), title},
        {QStringLiteral("comment"), s_timeshiftComment},
    };
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("conf"),
                      QString::fromUtf8(QJsonDocument(conf).toJson(QJsonDocument::Compact)));
    m_api->post(QStringLiteral("api/dvr/entry/create"),
                form,
                [this, channelUuid](const ApiResult &result) {
                    const QString dvrUuid = result.ok ? firstUuid(result.json()) : QString();

                    if (dvrUuid.isEmpty()) {
                        // Playback stays paused; resuming continues the live stream.
                        Q_EMIT notification(
                            tr("Could not record while paused: %1")
                                .arg(result.ok ? tr("no recording was created") : result.error));

                        return;
                    }

                    m_timeshiftUuid = dvrUuid;
                    Q_EMIT timeshiftStarted(channelUuid, dvrUuid);
                });
}

void TvhBackend::removeTimeshift(const QString &dvrUuid)
{
    if (dvrUuid.isEmpty()) {
        return;
    }

    if (dvrUuid == m_timeshiftUuid) {
        m_timeshiftUuid.clear();
    }

    // Deleting the entry also stops the recording and removes its file.
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("uuid"), dvrUuid);
    m_api->post(QStringLiteral("api/idnode/delete"), form, [](const ApiResult &) { });
}

void TvhBackend::removeStaleTimeshifts()
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("100000"));
    m_api->get(QStringLiteral("api/dvr/entry/grid"), query, [this](const ApiResult &result) {
        if (!result.ok) {
            return;
        }

        // Left over when Plasma TV quit or crashed while paused.
        for (const QJsonValue &value : entriesOf(result.json())) {
            const QJsonObject entry = value.toObject();
            const QString uuid = entry.value(QLatin1String("uuid")).toString();

            if (entry.value(QLatin1String("comment")).toString() == s_timeshiftComment
                && uuid != m_timeshiftUuid) {
                removeTimeshift(uuid);
            }
        }
    });
}

void TvhBackend::record(double eventId)
{
    const EpgEvent *event = m_store.byId(qint64(eventId));
    if (!event) {
        return;
    }

    const QString title = event->title;
    const qint64 id = event->id;
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("event_id"), QString::number(id));
    form.addQueryItem(QStringLiteral("config_uuid"), QString());
    m_api->post(
        QStringLiteral("api/dvr/entry/create_by_event"),
        form,
        [this, id, title](const ApiResult &result) {
            if (!result.ok) {
                Q_EMIT notification(tr("Could not schedule recording: %1").arg(result.error));
                return;
            }

            const EpgEvent *current = m_store.byId(id);
            const bool running = current && current->start <= QDateTime::currentSecsSinceEpoch();
            setRecordingState(id,
                              firstUuid(result.json()),
                              running ? QStringLiteral("recording") : QStringLiteral("scheduled"));
            Q_EMIT notification(tr("Recording scheduled: %1").arg(title));
        });
}

void TvhBackend::cancelRecording(double eventId)
{
    const EpgEvent *event = m_store.byId(qint64(eventId));
    if (!event || event->dvrUuid.isEmpty()) {
        return;
    }

    const qint64 id = event->id;
    const QString title = event->title;
    const bool running = event->dvrState == QLatin1String("recording");
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("uuid"), event->dvrUuid);
    const QString path =
        running ? QStringLiteral("api/dvr/entry/stop") : QStringLiteral("api/dvr/entry/cancel");
    m_api->post(path, form, [this, id, title, running](const ApiResult &result) {
        if (!result.ok) {
            Q_EMIT notification(tr("Could not cancel recording: %1").arg(result.error));
            return;
        }

        setRecordingState(id, {}, {});
        Q_EMIT notification(running ? tr("Recording stopped: %1").arg(title)
                                    : tr("Recording cancelled: %1").arg(title));
    });
}

void TvhBackend::setRecordingState(qint64 eventId, const QString &dvrUuid, const QString &dvrState)
{
    EpgEvent *event = m_store.byId(eventId);
    if (!event) {
        return;
    }

    event->dvrUuid = dvrUuid;
    event->dvrState = dvrState;
    m_database.updateRecording(eventId, dvrUuid, dvrState);
    m_channels->refreshNow();
    ++m_epgRevision;
    Q_EMIT epgChanged();
}

void TvhBackend::setStatus(Status status, const QString &text)
{
    if (status == m_status && text == m_statusText) {
        return;
    }

    m_status = status;
    m_statusText = text;
    Q_EMIT statusChanged();
}

void TvhBackend::setBusy(bool busy)
{
    if (busy == m_busy) {
        return;
    }

    m_busy = busy;
    Q_EMIT busyChanged();
}
