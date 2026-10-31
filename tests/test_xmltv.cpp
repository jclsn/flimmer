// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "xmltv.h"

#include <QDateTime>
#include <QTest>
#include <QTimeZone>

namespace
{

qint64 utc(int year, int month, int day, int hour, int minute)
{
    return QDateTime(QDate(year, month, day), QTime(hour, minute), QTimeZone::UTC)
        .toSecsSinceEpoch();
}

Xmltv::ChannelMap channelMap()
{
    Xmltv::ChannelMap map;
    map.byId.insert(QStringLiteral("das.erste.de"), {QStringLiteral("erste")});
    map.byName.insert(QStringLiteral("zdf hd"), {QStringLiteral("zdf")});

    return map;
}

const qint64 s_from = // NOLINT(bugprone-throwing-static-initialization)
    utc(2026, 10, 15, 0, 0);
const qint64 s_to = // NOLINT(bugprone-throwing-static-initialization)
    utc(2026, 10, 16, 0, 0);

class XmltvTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parseTime_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<qint64>("expected");

        QTest::newRow("utc offset") << "20261015203000 +0000" << utc(2026, 10, 15, 20, 30);
        QTest::newRow("positive offset") << "20261015203000 +0200" << utc(2026, 10, 15, 18, 30);
        QTest::newRow("negative offset") << "20261015203000 -0500" << utc(2026, 10, 16, 1, 30);
        QTest::newRow("no zone is utc") << "20261015203000" << utc(2026, 10, 15, 20, 30);
        QTest::newRow("no seconds") << "202610152030 +0000" << utc(2026, 10, 15, 20, 30);
        QTest::newRow("date only") << "20261015" << utc(2026, 10, 15, 0, 0);
        QTest::newRow("invalid date") << "20261345000000 +0000" << static_cast<qint64>(0);
        QTest::newRow("garbage") << "tomorrow" << static_cast<qint64>(0);
        QTest::newRow("empty") << "" << static_cast<qint64>(0);
    }

    void parseTime()
    {
        QFETCH(QString, text);
        QFETCH(qint64, expected);

        QCOMPARE(Xmltv::parseTime(text), expected);
    }

    void genreFromCategory_data()
    {
        QTest::addColumn<QString>("category");
        QTest::addColumn<int>("genre");

        QTest::newRow("movie") << "Spielfilm" << 0x10;
        QTest::newRow("news") << "Nachrichten" << 0x20;
        QTest::newRow("show") << "Quiz" << 0x30;
        QTest::newRow("sports") << "Fußball" << 0x40;
        QTest::newRow("kids before film") << "Kinderfilm" << 0x50;
        QTest::newRow("documentary before film") << "Dokumentarfilm" << 0x90;
        QTest::newRow("music") << "Konzert" << 0x60;
        QTest::newRow("umlaut") << "Komödie" << 0x10;
        QTest::newRow("unknown") << "Sonstiges" << 0;
    }

    void genreFromCategory()
    {
        QFETCH(QString, category);
        QFETCH(int, genre);

        QCOMPARE(Xmltv::genreFromCategory(category), genre);
    }

    void parsesMappedProgrammes()
    {
        const QByteArray data = R"(<?xml version="1.0" encoding="UTF-8"?>
<tv>
  <channel id="das.erste.de"><display-name>Das Erste</display-name></channel>
  <channel id="zdf.de"><display-name>ZDF</display-name><display-name>ZDF HD</display-name></channel>
  <channel id="unknown.de"><display-name>Unknown</display-name></channel>
  <programme start="20261015200000 +0200" stop="20261015201500 +0200" channel="das.erste.de">
    <title lang="de">Tagesschau</title>
    <title lang="en">News</title>
    <sub-title>Mit Wetter</sub-title>
    <desc>Die Nachrichten des Tages.</desc>
    <category>Nachrichten</category>
  </programme>
  <programme start="20261015201500 +0200" stop="20261015214500 +0200" channel="zdf.de">
    <title>Krimi</title>
  </programme>
  <programme start="20261015200000 +0200" stop="20261015210000 +0200" channel="unknown.de">
    <title>Not in the playlist</title>
  </programme>
</tv>)";

        QString error;
        const QVector<EpgEvent> events = Xmltv::parse(data, channelMap(), s_from, s_to, &error);

        QVERIFY(error.isEmpty());
        QCOMPARE(events.size(), 2);

        const EpgEvent &news = events.at(0);
        QCOMPARE(news.channel, QStringLiteral("erste"));
        QCOMPARE(news.start, utc(2026, 10, 15, 18, 0));
        QCOMPARE(news.stop, utc(2026, 10, 15, 18, 15));
        // The first translation is used.
        QCOMPARE(news.title, QStringLiteral("Tagesschau"));
        QCOMPARE(news.subtitle, QStringLiteral("Mit Wetter"));
        QCOMPARE(news.description, QStringLiteral("Die Nachrichten des Tages."));
        QCOMPARE(news.genre, 0x20);

        // Matched by its second display name.
        QCOMPARE(events.at(1).channel, QStringLiteral("zdf"));
        QVERIFY(events.at(0).id != events.at(1).id);
    }

    void fillsMissingStopTimes()
    {
        const QByteArray data = R"(<tv>
  <programme start="20261015200000 +0000" channel="das.erste.de"><title>One</title></programme>
  <programme start="20261015210000 +0000" stop="20261015220000 +0000" channel="das.erste.de"><title>Two</title></programme>
  <programme start="20261015230000 +0000" channel="das.erste.de"><title>Last</title></programme>
</tv>)";

        QString error;
        const QVector<EpgEvent> events = Xmltv::parse(data, channelMap(), s_from, s_to, &error);

        // The next programme ends the first; the last one has no end and is dropped.
        QCOMPARE(events.size(), 2);
        QCOMPARE(events.at(0).title, QStringLiteral("One"));
        QCOMPARE(events.at(0).stop, utc(2026, 10, 15, 21, 0));
        QCOMPARE(events.at(1).title, QStringLiteral("Two"));
    }

    void keepsOnlyTheRequestedWindow()
    {
        const QByteArray data = R"(<tv>
  <programme start="20261014220000 +0000" stop="20261014230000 +0000" channel="das.erste.de"><title>Over</title></programme>
  <programme start="20261014233000 +0000" stop="20261015003000 +0000" channel="das.erste.de"><title>Running</title></programme>
  <programme start="20261016000000 +0000" stop="20261016010000 +0000" channel="das.erste.de"><title>Later</title></programme>
</tv>)";

        QString error;
        const QVector<EpgEvent> events = Xmltv::parse(data, channelMap(), s_from, s_to, &error);

        QCOMPARE(events.size(), 1);
        QCOMPARE(events.first().title, QStringLiteral("Running"));
    }

    void matchesProgrammesWithoutChannelElement()
    {
        const QByteArray data = R"(<tv>
  <programme start="20261015200000 +0000" stop="20261015210000 +0000" channel="ZDF HD"><title>By name</title></programme>
</tv>)";

        QString error;
        const QVector<EpgEvent> events = Xmltv::parse(data, channelMap(), s_from, s_to, &error);

        QCOMPARE(events.size(), 1);
        QCOMPARE(events.first().channel, QStringLiteral("zdf"));
    }

    void sharesGuideChannels()
    {
        Xmltv::ChannelMap map;
        map.byId.insert(QStringLiteral("zdf.de"), {QStringLiteral("zdf"), QStringLiteral("zdf#2")});
        const QByteArray data = R"(<tv>
  <programme start="20261015200000 +0000" stop="20261015210000 +0000" channel="zdf.de"><title>Both</title></programme>
</tv>)";

        QString error;
        const QVector<EpgEvent> events = Xmltv::parse(data, map, s_from, s_to, &error);

        QCOMPARE(events.size(), 2);
        QCOMPARE(events.at(0).channel, QStringLiteral("zdf"));
        QCOMPARE(events.at(1).channel, QStringLiteral("zdf#2"));
    }

    void reportsErrors_data()
    {
        QTest::addColumn<QByteArray>("data");

        QTest::newRow("gzip") << QByteArray("\x1F\x8B\x08\x00", 4);
        QTest::newRow("not xmltv") << QByteArray("<html><body>Login</body></html>");
        QTest::newRow("broken xml") << QByteArray("<tv><programme start=");
    }

    void reportsErrors()
    {
        QFETCH(QByteArray, data);

        QString error;
        const QVector<EpgEvent> events = Xmltv::parse(data, channelMap(), s_from, s_to, &error);

        QVERIFY(events.isEmpty());
        QVERIFY(!error.isEmpty());
    }
};

}

QTEST_GUILESS_MAIN(XmltvTest)

#include "test_xmltv.moc"
