// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "epgstore.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

namespace
{
EpgEvent makeEvent(qint64 id, const QString &channel, qint64 start, qint64 stop)
{
    EpgEvent result;
    result.id = id;
    result.channel = channel;
    result.start = start;
    result.stop = stop;
    result.title = QStringLiteral("Event %1").arg(id);

    return result;
}

const QString s_one = // NOLINT(bugprone-throwing-static-initialization)
    QStringLiteral("one");
const QString s_two = // NOLINT(bugprone-throwing-static-initialization)
    QStringLiteral("two");

class EpgStoreTest : public QObject
{
    Q_OBJECT

private:
    EpgStore m_store;

private Q_SLOTS:
    void init()
    {
        // Out of order on purpose; the store sorts by start time.
        m_store.setEvents({
            makeEvent(3, s_one, 300, 400),
            makeEvent(1, s_one, 100, 200),
            makeEvent(2, s_one, 200, 300),
            makeEvent(10, s_two, 150, 250),
        });
    }

    void counts()
    {
        QCOMPARE(m_store.size(), 4);
        QVERIFY(!m_store.isEmpty());
        QCOMPARE(m_store.lastStop(), 400);

        m_store.clear();
        QVERIFY(m_store.isEmpty());
        QCOMPARE(m_store.size(), 0);
        QCOMPARE(m_store.lastStop(), 0);
    }

    void eventAt()
    {
        QCOMPARE(m_store.eventAt(s_one, 100)->id, 1);
        QCOMPARE(m_store.eventAt(s_one, 199)->id, 1);
        // The stop time belongs to the next event.
        QCOMPARE(m_store.eventAt(s_one, 200)->id, 2);
        QCOMPARE(m_store.eventAt(s_two, 200)->id, 10);

        QVERIFY(!m_store.eventAt(s_one, 99));
        QVERIFY(!m_store.eventAt(s_one, 400));
        QVERIFY(!m_store.eventAt(QStringLiteral("unknown"), 150));
    }

    void nextAfter()
    {
        QCOMPARE(m_store.nextAfter(s_one, 100)->id, 2);
        QCOMPARE(m_store.nextAfter(s_one, 0)->id, 1);
        QVERIFY(!m_store.nextAfter(s_one, 300));
    }

    void range()
    {
        const auto events = m_store.range(s_one, 150, 350);

        QCOMPARE(events.size(), 3);
        QCOMPARE(events.first()->id, 1);
        QCOMPARE(events.last()->id, 3);

        // Events ending at 'from' or starting at 'to' are outside.
        QCOMPARE(m_store.range(s_one, 200, 300).size(), 1);
        QVERIFY(m_store.range(s_one, 400, 500).isEmpty());
    }

    void byId()
    {
        QCOMPARE(m_store.byId(10)->channel, s_two);
        QVERIFY(!m_store.byId(99));

        // Recording state is changed in place.
        m_store.byId(2)->dvrState = QStringLiteral("scheduled");
        QCOMPARE(m_store.eventAt(s_one, 250)->dvrState, QStringLiteral("scheduled"));
    }

    void dropsDuplicates()
    {
        // Some EPG grabbers list a programme twice.
        m_store.setEvents({makeEvent(1, s_one, 100, 200), makeEvent(2, s_one, 100, 200)});

        QCOMPARE(m_store.size(), 1);
    }

    void cutsOverlappingEvents()
    {
        m_store.setEvents({makeEvent(1, s_one, 100, 200), makeEvent(2, s_one, 150, 250)});

        QCOMPARE(m_store.size(), 2);
        QCOMPARE(m_store.byId(1)->stop, 150);
        QCOMPARE(m_store.eventAt(s_one, 160)->id, 2);
    }

    void dropsInvalidEvents()
    {
        m_store.setEvents({
            makeEvent(1, QString(), 100, 200),
            makeEvent(2, s_one, 300, 300),
            makeEvent(3, s_one, 400, 300),
        });

        QVERIFY(m_store.isEmpty());
    }

    static void fromJson()
    {
        const QJsonObject object{
            {QStringLiteral("eventId"), 42},
            {QStringLiteral("channelUuid"), s_one},
            {QStringLiteral("start"), 1000},
            {QStringLiteral("stop"), 2000},
            {QStringLiteral("title"), QStringLiteral("Tagesschau")},
            {QStringLiteral("genre"), QJsonArray{0x21, 0x10}},
            {QStringLiteral("dvrState"), QStringLiteral("recording")},
        };

        const EpgEvent parsed = EpgEvent::fromJson(object);

        QCOMPARE(parsed.id, 42);
        QCOMPARE(parsed.channel, s_one);
        QCOMPARE(parsed.stop, 2000);
        QCOMPARE(parsed.title, QStringLiteral("Tagesschau"));
        // Only the first genre is kept.
        QCOMPARE(parsed.genre, 0x21);
        QCOMPARE(parsed.dvrState, QStringLiteral("recording"));
    }

    static void toVariant()
    {
        EpgEvent news = makeEvent(7, s_one, 1000, 2000);
        news.genre = 0x23;

        const QVariantMap map = news.toVariant();

        // Times are in milliseconds for JavaScript dates.
        QCOMPARE(map.value(QStringLiteral("start")).toDouble(), 1000000.0);
        QCOMPARE(map.value(QStringLiteral("stop")).toDouble(), 2000000.0);
        // The genre is reduced to its main group.
        QCOMPARE(map.value(QStringLiteral("genre")).toInt(), 0x20);
        QCOMPARE(map.value(QStringLiteral("genreName")).toString(), QStringLiteral("News"));
    }
};

}

QTEST_GUILESS_MAIN(EpgStoreTest)

#include "test_epgstore.moc"
