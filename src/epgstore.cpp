// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "epgstore.h"

#include <QCoreApplication>
#include <QJsonArray>

#include <algorithm>

Channel Channel::fromJson(const QJsonObject &object)
{
    Channel channel;
    channel.uuid = object.value(QLatin1String("uuid")).toString();
    channel.number = object.value(QLatin1String("number")).toInt();
    channel.name = object.value(QLatin1String("name")).toString();
    channel.icon = object.value(QLatin1String("icon_public_url")).toString();
    return channel;
}

EpgEvent EpgEvent::fromJson(const QJsonObject &object)
{
    EpgEvent event;
    event.id = object.value(QLatin1String("eventId")).toInteger();
    event.channel = object.value(QLatin1String("channelUuid")).toString();
    event.start = object.value(QLatin1String("start")).toInteger();
    event.stop = object.value(QLatin1String("stop")).toInteger();
    event.title = object.value(QLatin1String("title")).toString();
    event.subtitle = object.value(QLatin1String("subtitle")).toString();
    event.summary = object.value(QLatin1String("summary")).toString();
    event.description = object.value(QLatin1String("description")).toString();

    const QJsonArray genres = object.value(QLatin1String("genre")).toArray();
    if (!genres.isEmpty()) {
        event.genre = genres.first().toInt();
    }

    event.dvrUuid = object.value(QLatin1String("dvrUuid")).toString();
    event.dvrState = object.value(QLatin1String("dvrState")).toString();
    return event;
}

QVariantMap EpgEvent::toVariant() const
{
    // Times are handed to QML in milliseconds so they plug straight into Date.
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("channel"), channel},
        {QStringLiteral("start"), static_cast<double>(start) * 1000.0},
        {QStringLiteral("stop"), static_cast<double>(stop) * 1000.0},
        {QStringLiteral("title"), title},
        {QStringLiteral("subtitle"), subtitle},
        {QStringLiteral("summary"), summary},
        {QStringLiteral("description"), description},
        {QStringLiteral("genre"), genre & 0xF0},
        {QStringLiteral("genreName"), genreName(genre)},
        {QStringLiteral("dvrUuid"), dvrUuid},
        {QStringLiteral("dvrState"), dvrState},
    };
}

QString genreName(int genre)
{
    switch (genre & 0xF0) {
    case 0x10:
        return QCoreApplication::translate("Genre", "Movie");
    case 0x20:
        return QCoreApplication::translate("Genre", "News");
    case 0x30:
        return QCoreApplication::translate("Genre", "Show");
    case 0x40:
        return QCoreApplication::translate("Genre", "Sports");
    case 0x50:
        return QCoreApplication::translate("Genre", "Kids");
    case 0x60:
        return QCoreApplication::translate("Genre", "Music");
    case 0x70:
        return QCoreApplication::translate("Genre", "Arts & Culture");
    case 0x80:
        return QCoreApplication::translate("Genre", "Society");
    case 0x90:
        return QCoreApplication::translate("Genre", "Education");
    case 0xA0:
        return QCoreApplication::translate("Genre", "Leisure");
    default:
        return {};
    }
}

void EpgStore::clear()
{
    m_byChannel.clear();
    m_index.clear();
    m_lastStop = 0;
    m_size = 0;
}

void EpgStore::setEvents(QVector<EpgEvent> events)
{
    clear();
    std::ranges::sort(events, [](const EpgEvent &a, const EpgEvent &b) {
        return a.start < b.start;
    });

    for (EpgEvent &event : events) {
        if (event.channel.isEmpty() || event.stop <= event.start) {
            continue;
        }

        m_lastStop = std::max(m_lastStop, event.stop);
        auto &list = m_byChannel[event.channel];

        // Drop overlapping duplicates that some EPG grabbers produce.
        if (!list.isEmpty() && list.last().stop > event.start) {
            if (list.last().start == event.start) {
                continue;
            }

            list.last().stop = event.start;
        }

        list.append(std::move(event));
    }

    for (auto it = m_byChannel.cbegin(); it != m_byChannel.cend(); ++it) {
        for (int i = 0; i < it.value().size(); ++i) {
            m_index.insert(it.value().at(i).id, {it.key(), i});
        }

        m_size += it.value().size();
    }
}

const EpgEvent *EpgStore::eventAt(const QString &channel, qint64 time) const
{
    const auto it = m_byChannel.constFind(channel);
    if (it == m_byChannel.cend()) {
        return nullptr;
    }

    const QList<EpgEvent> &list = it.value();
    auto position = std::ranges::upper_bound(list, time, std::ranges::less{}, &EpgEvent::start);

    if (position == list.cbegin()) {
        return nullptr;
    }

    --position;
    return time < position->stop ? &*position : nullptr;
}

const EpgEvent *EpgStore::nextAfter(const QString &channel, qint64 time) const
{
    const auto it = m_byChannel.constFind(channel);
    if (it == m_byChannel.cend()) {
        return nullptr;
    }

    const auto &list = it.value();
    auto const position =
        std::ranges::upper_bound(list, time, std::ranges::less{}, &EpgEvent::start);

    return position == list.cend() ? nullptr : &*position;
}

const EpgEvent *EpgStore::byId(qint64 id) const
{
    const auto it = m_index.constFind(id);
    if (it == m_index.cend()) {
        return nullptr;
    }

    return &m_byChannel[it->first].at(it->second);
}

EpgEvent *EpgStore::byId(qint64 id)
{
    const auto it = m_index.constFind(id);
    if (it == m_index.cend()) {
        return nullptr;
    }

    return &m_byChannel[it->first][it->second];
}

QVector<const EpgEvent *> EpgStore::range(const QString &channel, qint64 from, qint64 to) const
{
    QVector<const EpgEvent *> result;
    const auto it = m_byChannel.constFind(channel);
    if (it == m_byChannel.cend()) {
        return result;
    }

    const auto &list = it.value();
    // First event that ends after 'from'; stop times are sorted as events don't overlap.
    auto position = std::ranges::upper_bound(list, from, std::ranges::less{}, &EpgEvent::stop);

    for (; position != list.cend() && position->start < to; ++position) {
        result.append(&*position);
    }

    return result;
}
