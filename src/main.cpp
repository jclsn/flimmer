#include "appsettings.h"
#include "networkauth.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QQmlApplicationEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QStyleFactory>

namespace
{
// Network access for QML (channel logos): authenticates against TVHeadend's
// image cache and keeps logos on disk so the channel list renders instantly.
class AuthenticatedNamFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override
    {
        auto *nam = new QNetworkAccessManager(parent);
        auto *cache = new QNetworkDiskCache(nam);
        cache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                 + QStringLiteral("/images"));
        cache->setMaximumCacheSize(64 * 1024 * 1024);
        nam->setCache(cache);
        NetworkAuth::install(nam);
        return nam;
    }
};
}

int main(int argc, char *argv[])
{
    // QApplication (not QGuiApplication): the Plasma Qt Quick Controls style
    // (org.kde.desktop) paints through the widget style.
    QApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("plasma-tv"));
    QGuiApplication::setApplicationName(QStringLiteral("plasma-tv"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Plasma TV"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QGuiApplication::setDesktopFileName(QStringLiteral("plasma-tv"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Fullscreen TVHeadend client for the living room"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption windowed({QStringLiteral("w"), QStringLiteral("windowed")},
                                QStringLiteral("Start in a window instead of fullscreen"));
    parser.addOption(windowed);
    parser.process(app);

    // Follow Plasma's look; outside a Plasma session fall back to Breeze explicitly.
    const bool plasmaSession =
        qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QLatin1String("KDE"));
    if (!plasmaSession
        && QStyleFactory::keys().contains(QStringLiteral("Breeze"), Qt::CaseInsensitive)) {
        QApplication::setStyle(QStringLiteral("Breeze"));
    }

    if (QIcon::themeName().isEmpty() || QIcon::themeName() == QLatin1String("hicolor")) {
        QIcon::setThemeName(QStringLiteral("breeze"));
    }

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
        QQuickStyle::setFallbackStyle(QStringLiteral("Fusion"));
    }

    AppSettings::instance()->applyColorScheme();

    AuthenticatedNamFactory namFactory;
    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(&namFactory);
    engine.setInitialProperties({{QStringLiteral("startWindowed"), parser.isSet(windowed)}});
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            QCoreApplication::exit(1);
        },
        Qt::QueuedConnection);
    engine.loadFromModule("PlasmaTv", "Main");

    return app.exec();
}
