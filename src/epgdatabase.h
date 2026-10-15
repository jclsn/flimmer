/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef EPGDATABASE_H
#define EPGDATABASE_H

#include "epgstore.h"

#include <QSqlDatabase>
#include <QString>
#include <QVector>

// SQLite cache of channels and EPG data so the UI is populated instantly on
// start-up and stays usable while the server is unreachable.
class EpgDatabase
{
public:
    EpgDatabase();
    ~EpgDatabase();

    bool open();
    QString serverIdentity() const;
    void setServerIdentity(const QString &identity);
    qint64 lastUpdate() const;

    void clear();
    void saveChannels(const QVector<Channel> &channels);
    QVector<Channel> loadChannels() const;
    void saveEvents(const QVector<EpgEvent> &events, qint64 updatedAt);
    QVector<EpgEvent> loadEvents(qint64 notEndedBefore) const;
    void updateRecording(qint64 eventId, const QString &dvrUuid, const QString &dvrState);

private:
    QString meta(const QString &key) const;
    void setMeta(const QString &key, const QString &value);

    QString m_connectionName;
};

#endif // EPGDATABASE_H
