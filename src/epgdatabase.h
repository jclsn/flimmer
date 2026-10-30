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
    [[nodiscard]] QString serverIdentity() const;
    void setServerIdentity(const QString &identity);
    [[nodiscard]] qint64 lastUpdate() const;

    void clear();
    void saveChannels(const QVector<Channel> &channels);
    [[nodiscard]] QVector<Channel> loadChannels() const;
    void saveEvents(const QVector<EpgEvent> &events, qint64 updatedAt);
    [[nodiscard]] QVector<EpgEvent> loadEvents(qint64 notEndedBefore) const;
    void updateRecording(qint64 eventId, const QString &dvrUuid, const QString &dvrState);

private:
    [[nodiscard]] QString meta(const QString &key) const;
    void setMeta(const QString &key, const QString &value);

    QString m_connectionName;
};

#endif // EPGDATABASE_H
