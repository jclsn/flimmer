/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef EPGDATABASE_H
#define EPGDATABASE_H

#include "epgstore.h"

#include <QObject>
#include <QString>
#include <QThread>
#include <QVector>

#include <functional>

// SQLite cache of channels and EPG data so the UI is populated instantly on
// start-up and stays usable while the server is unreachable.
//
// All database work runs in order on a thread of its own: saving a guide of
// several days takes long enough to make playback and the remote stutter.
class EpgDatabase
{
public:
    EpgDatabase();
    ~EpgDatabase();

    bool open();
    void clear();

    [[nodiscard]] QString serverIdentity() const
    {
        return m_serverIdentity;
    }

    void setServerIdentity(const QString &identity);
    [[nodiscard]] qint64 lastUpdate() const;

    void saveChannels(const QVector<Channel> &channels) const;
    [[nodiscard]] QVector<Channel> loadChannels() const;

    void saveEvents(const QVector<EpgEvent> &events, qint64 updatedAt) const;

    // Calls 'done' on the thread of 'context' once the events are loaded. 'context' must
    // outlive the database.
    void loadEvents(qint64 notEndedBefore,
                    QObject *context,
                    std::function<void(QVector<EpgEvent>)> const &done) const;

    void updateRecording(qint64 eventId, const QString &dvrUuid, const QString &dvrState) const;

private:
    // Runs 'job' on the database thread after everything queued before.
    void run(std::function<void()> job) const;

    // Like run(), but waits for 'job' to finish.
    void runAndWait(std::function<void()> job) const;

    [[nodiscard]] QString meta(const QString &key) const;
    void setMeta(const QString &key, const QString &value) const;

    QString m_connectionName;
    QString m_serverIdentity;
    QThread m_thread;

    // Lives on m_thread; the jobs are queued to it.
    QObject *m_worker;
};

#endif // EPGDATABASE_H
