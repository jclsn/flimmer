#include "screensaverinhibitor.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

namespace
{
const QString s_service = QStringLiteral("org.freedesktop.ScreenSaver");
const QString s_path = QStringLiteral("/ScreenSaver");
}

ScreenSaverInhibitor::ScreenSaverInhibitor(QObject *parent)
    : QObject(parent)
{
}

ScreenSaverInhibitor::~ScreenSaverInhibitor()
{
    uninhibit();
}

void ScreenSaverInhibitor::setActive(bool active)
{
    if (active == m_active) {
        return;
    }

    m_active = active;

    if (active) {
        inhibit();
    } else {
        uninhibit();
    }

    Q_EMIT activeChanged();
}

void ScreenSaverInhibitor::inhibit()
{
    QDBusMessage message =
        QDBusMessage::createMethodCall(s_service, s_path, s_service, QStringLiteral("Inhibit"));
    message << QStringLiteral("plasma-tv") << tr("Watching live TV");
    auto *watcher =
        new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        QDBusPendingReply<uint> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            return;
        }

        m_cookie = reply.value();

        if (!m_active) { // deactivated while the call was pending
            uninhibit();
        }
    });
}

void ScreenSaverInhibitor::uninhibit()
{
    if (m_cookie == 0) {
        return;
    }

    QDBusMessage message =
        QDBusMessage::createMethodCall(s_service, s_path, s_service, QStringLiteral("UnInhibit"));
    message << m_cookie;
    QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
    m_cookie = 0;
}
