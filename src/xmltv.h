/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef XMLTV_H
#define XMLTV_H

#include "epgstore.h"

#include <QByteArray>
#include <QHash>
#include <QStringList>
#include <QVector>

#include <functional>

class QObject;

namespace Xmltv
{
// Maps guide channels to playlist channels. Keys are lower case; several
// playlist channels (e.g. SD and HD) may share one guide channel.
struct ChannelMap {
    QHash<QString, QStringList> byId; // tvg-id → channel uuids
    QHash<QString, QStringList> byName; // tvg-name / channel name → channel uuids
};

// Parses an XMLTV document and returns the programmes of mapped channels that
// overlap [from, to). Pure function, safe to run on a worker thread.
QVector<EpgEvent>
parse(const QByteArray &data, const ChannelMap &map, qint64 from, qint64 to, QString *error);

// Runs parse() on a thread of its own, as guides of large playlists are tens of megabytes,
// and calls 'done' on the thread of 'context' with the events or an error.
using Done = std::function<void(const QVector<EpgEvent> &events, const QString &error)>;
void parseInThread(QObject *context,
                   const QByteArray &data,
                   const ChannelMap &map,
                   qint64 from,
                   qint64 to,
                   const Done &done);

// Parses an XMLTV time ("20261015203000 +0200") into seconds since epoch, 0 on failure.
qint64 parseTime(QStringView text);

// Maps XMLTV categories (free text) to an ETSI EN 300 468 content nibble.
int genreFromCategory(const QString &category);
}

#endif // XMLTV_H
