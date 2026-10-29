// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "serverconfig.h"

#include <QTest>

class ServerConfigTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseUrl_data()
    {
        QTest::addColumn<QString>("host");
        QTest::addColumn<int>("port");
        QTest::addColumn<bool>("https");
        QTest::addColumn<QString>("expected");

        QTest::newRow("plain host")
            << "192.168.1.10" << 9981 << false << "http://192.168.1.10:9981/";
        QTest::newRow("https") << "tv.example.org" << 443 << true << "https://tv.example.org:443/";
        QTest::newRow("whitespace") << "  tvh  " << 9981 << false << "http://tvh:9981/";
        QTest::newRow("scheme in host") << "https://tvh" << 9981 << false << "https://tvh:9981/";
        QTest::newRow("port in host") << "tvh:8080" << 9981 << false << "http://tvh:8080/";
        QTest::newRow("web root") << "http://tvh:9981/tvheadend/" << 1 << false
                                  << "http://tvh:9981/tvheadend/";
        QTest::newRow("ipv6") << "[fe80::1]" << 9981 << false << "http://[fe80::1]:9981/";
        QTest::newRow("ipv6 with port")
            << "[fe80::1]:8080" << 9981 << false << "http://[fe80::1]:8080/";
        QTest::newRow("empty") << "" << 9981 << false << "";
    }

    void baseUrl()
    {
        QFETCH(QString, host);
        QFETCH(int, port);
        QFETCH(bool, https);
        QFETCH(QString, expected);

        ServerConfig config;
        config.host = host;
        config.port = port;
        config.https = https;

        QCOMPARE(config.baseUrl().toString(), expected);
        QCOMPARE(config.isValid(), !expected.isEmpty());
    }

    void urlResolvesAgainstWebRoot()
    {
        ServerConfig config;
        config.host = QStringLiteral("tvh/tvheadend");

        QCOMPARE(config.url(QStringLiteral("/api/serverinfo")).toString(),
                 QStringLiteral("http://tvh:9981/tvheadend/api/serverinfo"));
        QCOMPARE(config.url(QStringLiteral("stream/channel/abc")).path(),
                 QStringLiteral("/tvheadend/stream/channel/abc"));
    }

    void identityLeavesOutCredentials()
    {
        ServerConfig config;
        config.host = QStringLiteral("tvh");
        config.username = QStringLiteral("user");
        config.password = QStringLiteral("secret");

        QVERIFY(!config.identity().contains(QLatin1String("secret")));
        QVERIFY(!config.identity().contains(QLatin1String("user")));
    }

    void backendNames()
    {
        QCOMPARE(ServerConfig::backendFromString(QStringLiteral("m3u")),
                 ServerConfig::Backend::M3u);
        QCOMPARE(ServerConfig::backendFromString(QStringLiteral("tvheadend")),
                 ServerConfig::Backend::Tvheadend);
        // Unknown or missing values fall back to TVHeadend.
        QCOMPARE(ServerConfig::backendFromString(QString()), ServerConfig::Backend::Tvheadend);
        QCOMPARE(ServerConfig::backendName(ServerConfig::Backend::M3u), QStringLiteral("m3u"));
    }

    void m3uSource()
    {
        ServerConfig config;
        config.backend = ServerConfig::Backend::M3u;
        config.playlistUrl = QStringLiteral("192.168.1.10:9191/output/m3u");

        QVERIFY(config.isValid());
        QCOMPARE(config.playlist().toString(),
                 QStringLiteral("http://192.168.1.10:9191/output/m3u"));
        // The playlist's server is used for the credentials and logos.
        QCOMPARE(config.baseUrl().toString(), QStringLiteral("http://192.168.1.10:9191/"));
        QVERIFY(config.identity().startsWith(QLatin1String("m3u:")));
    }

    void m3uRejectsOtherSchemes()
    {
        ServerConfig config;
        config.backend = ServerConfig::Backend::M3u;
        config.playlistUrl = QStringLiteral("ftp://example.org/list.m3u");

        QVERIFY(!config.isValid());

        config.playlistUrl = QStringLiteral("/home/user/list.m3u");
        QVERIFY(config.isValid());
        QVERIFY(config.playlist().isLocalFile());
    }
};

QTEST_GUILESS_MAIN(ServerConfigTest)

#include "test_serverconfig.moc"
