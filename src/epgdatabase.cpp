// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "epgdatabase.h"

#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace
{
constexpr int s_schemaVersion = 3;
}

EpgDatabase::EpgDatabase()
    : m_connectionName(QStringLiteral("epg-cache"))
    , m_worker(new QObject)
{
    m_thread.setObjectName(QStringLiteral("EPG cache"));
    m_worker->moveToThread(&m_thread);
    m_thread.start();
}

EpgDatabase::~EpgDatabase()
{
    // Queued last, so pending writes are finished first.
    run([this]() {
        {
            QSqlDatabase database = QSqlDatabase::database(m_connectionName, false);

            if (database.isOpen()) {
                database.close();
            }
        }

        QSqlDatabase::removeDatabase(m_connectionName);
        delete m_worker;
        QThread::currentThread()->quit();
    });
    m_thread.wait();
}

void EpgDatabase::run(std::function<void()> job) const
{
    QMetaObject::invokeMethod(m_worker, std::move(job), Qt::QueuedConnection);
}

void EpgDatabase::runAndWait(std::function<void()> job) const
{
    QMetaObject::invokeMethod(m_worker, std::move(job), Qt::BlockingQueuedConnection);
}

bool EpgDatabase::open()
{
    bool ok = false;

    runAndWait([this, &ok]() {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);

        QSqlDatabase database =
            QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
        database.setDatabaseName(dir + QStringLiteral("/epg-cache.sqlite"));

        if (!database.open()) {
            qWarning() << "Cannot open EPG cache:" << database.lastError().text();

            return;
        }

        QSqlQuery query(database);
        query.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
        query.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
        query.exec(QStringLiteral("PRAGMA user_version"));

        const int version = query.next() ? query.value(0).toInt() : 0;
        if (version == 1) {
            // Version 2 added the stream URL of M3U channels.
            query.exec(QStringLiteral("ALTER TABLE channels ADD COLUMN stream TEXT"));
        } else if (version != s_schemaVersion && version != 2) {
            query.exec(QStringLiteral("DROP TABLE IF EXISTS channels"));
            query.exec(QStringLiteral("DROP TABLE IF EXISTS events"));
            query.exec(QStringLiteral("DROP TABLE IF EXISTS meta"));
        }

        const QStringList schema = {
            QStringLiteral("CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS channels (uuid TEXT PRIMARY KEY, number INTEGER, "
                "name TEXT, icon TEXT, position INTEGER, stream TEXT)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS events (id INTEGER PRIMARY KEY, channel TEXT NOT NULL, "
                "start INTEGER NOT NULL, stop INTEGER NOT NULL, title TEXT, subtitle TEXT, summary "
                "TEXT, "
                "description TEXT, genre INTEGER, dvr_uuid TEXT, dvr_state TEXT)"),
            // Version 3: events are always loaded as a whole, so the index only slowed down
            // saving them.
            QStringLiteral("DROP INDEX IF EXISTS events_channel_start"),
            QStringLiteral("PRAGMA user_version = %1").arg(s_schemaVersion),
        };

        for (const QString &statement : schema) {
            if (!query.exec(statement)) {
                qWarning() << "EPG cache schema error:" << query.lastError().text();

                return;
            }
        }

        m_serverIdentity = meta(QStringLiteral("server"));
        ok = true;
    });

    return ok;
}

QString EpgDatabase::meta(const QString &key) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT value FROM meta WHERE key = ?"));
    query.addBindValue(key);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }

    return {};
}

void EpgDatabase::setMeta(const QString &key, const QString &value) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO meta (key, value) VALUES (?, ?)"));
    query.addBindValue(key);
    query.addBindValue(value);
    query.exec();
}

void EpgDatabase::setServerIdentity(const QString &identity)
{
    m_serverIdentity = identity;

    run([this, identity]() {
        setMeta(QStringLiteral("server"), identity);
    });
}

qint64 EpgDatabase::lastUpdate() const
{
    qint64 result = 0;

    runAndWait([this, &result]() {
        result = meta(QStringLiteral("lastUpdate")).toLongLong();
    });

    return result;
}

void EpgDatabase::clear()
{
    m_serverIdentity.clear();

    run([this]() {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        QSqlQuery query(database);
        database.transaction();
        query.exec(QStringLiteral("DELETE FROM channels"));
        query.exec(QStringLiteral("DELETE FROM events"));
        query.exec(QStringLiteral("DELETE FROM meta"));
        database.commit();
    });
}

void EpgDatabase::saveChannels(const QVector<Channel> &channels) const
{
    run([this, channels]() {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        database.transaction();
        QSqlQuery query(database);
        query.exec(QStringLiteral("DELETE FROM channels"));
        query.prepare(
            QStringLiteral("INSERT INTO channels (uuid, number, name, icon, position, stream) "
                           "VALUES (?, ?, ?, ?, ?, ?)"));

        for (int i = 0; i < channels.size(); ++i) {
            const Channel &channel = channels.at(i);
            query.addBindValue(channel.uuid);
            query.addBindValue(channel.number);
            query.addBindValue(channel.name);
            query.addBindValue(channel.icon);
            query.addBindValue(i);
            query.addBindValue(channel.stream);
            query.exec();
        }

        database.commit();
    });
}

QVector<Channel> EpgDatabase::loadChannels() const
{
    QVector<Channel> result;

    runAndWait([this, &result]() {
        QSqlQuery query(QSqlDatabase::database(m_connectionName));

        if (!query.exec(QStringLiteral(
                "SELECT uuid, number, name, icon, stream FROM channels ORDER BY position"))) {
            return;
        }

        while (query.next()) {
            result.append({
                .uuid = query.value(0).toString(),
                .number = query.value(1).toInt(),
                .name = query.value(2).toString(),
                .icon = query.value(3).toString(),
                .stream = query.value(4).toString(),
            });
        }
    });

    return result;
}

void EpgDatabase::saveEvents(const QVector<EpgEvent> &events, qint64 updatedAt) const
{
    run([this, events, updatedAt]() {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        database.transaction();
        QSqlQuery query(database);
        query.exec(QStringLiteral("DELETE FROM events"));
        query.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO events (id, channel, start, stop, title, subtitle, summary, "
            "description, genre, dvr_uuid, dvr_state) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

        for (const EpgEvent &event : events) {
            query.addBindValue(event.id);
            query.addBindValue(event.channel);
            query.addBindValue(event.start);
            query.addBindValue(event.stop);
            query.addBindValue(event.title);
            query.addBindValue(event.subtitle);
            query.addBindValue(event.summary);
            query.addBindValue(event.description);
            query.addBindValue(event.genre);
            query.addBindValue(event.dvrUuid);
            query.addBindValue(event.dvrState);
            query.exec();
        }

        setMeta(QStringLiteral("lastUpdate"), QString::number(updatedAt));
        database.commit();
    });
}

void EpgDatabase::loadEvents(qint64 notEndedBefore,
                             QObject *context,
                             std::function<void(QVector<EpgEvent>)> const &done) const
{
    run([this, notEndedBefore, context, done]() {
        QVector<EpgEvent> result;
        QSqlQuery query(QSqlDatabase::database(m_connectionName));
        query.setForwardOnly(true);
        query.prepare(QStringLiteral(
            "SELECT id, channel, start, stop, title, subtitle, summary, description, genre, "
            "dvr_uuid, dvr_state FROM events WHERE stop > ?"));
        query.addBindValue(notEndedBefore);

        if (query.exec()) {
            while (query.next()) {
                EpgEvent event;
                event.id = query.value(0).toLongLong();
                event.channel = query.value(1).toString();
                event.start = query.value(2).toLongLong();
                event.stop = query.value(3).toLongLong();
                event.title = query.value(4).toString();
                event.subtitle = query.value(5).toString();
                event.summary = query.value(6).toString();
                event.description = query.value(7).toString();
                event.genre = query.value(8).toInt();
                event.dvrUuid = query.value(9).toString();
                event.dvrState = query.value(10).toString();
                result.append(std::move(event));
            }
        }

        QMetaObject::invokeMethod(
            context,
            [done, result = std::move(result)]() {
                done(result);
            },
            Qt::QueuedConnection);
    });
}

void EpgDatabase::updateRecording(qint64 eventId,
                                  const QString &dvrUuid,
                                  const QString &dvrState) const
{
    run([this, eventId, dvrUuid, dvrState]() {
        QSqlQuery query(QSqlDatabase::database(m_connectionName));
        query.prepare(QStringLiteral("UPDATE events SET dvr_uuid = ?, dvr_state = ? WHERE id = ?"));
        query.addBindValue(dvrUuid);
        query.addBindValue(dvrState);
        query.addBindValue(eventId);
        query.exec();
    });
}
