// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "inputdebug.h"

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

namespace
{
Q_LOGGING_CATEGORY(lcInput, "flimmer.input", QtWarningMsg)

constexpr int s_stallCheckMs = 100;
constexpr qint64 s_stallReportMs = 500;

QString describe(const QQuickItem *item)
{
    if (!item) {
        return QStringLiteral("<nothing>");
    }

    QStringList chain;
    for (const QQuickItem *it = item; it && chain.size() < 4; it = it->parentItem()) {
        QString name = QString::fromLatin1(it->metaObject()->className());
        name =
            name.section(QStringLiteral("_QMLTYPE_"), 0, 0).section(QStringLiteral("_QML_"), 0, 0);

        if (!it->objectName().isEmpty()) {
            name += QLatin1Char('#') + it->objectName();
        }

        chain.append(name);
    }

    return chain.join(QStringLiteral(" < "))
        + (item->isVisible() ? QString() : QStringLiteral(" [HIDDEN]"));
}

class KeyLogger : public QObject
{
public:
    explicit KeyLogger(QQuickWindow *window)
        : QObject(window)
        , m_window(window)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window && event->type() == QEvent::KeyPress) {
            const auto *key = static_cast<QKeyEvent *>(event);
            qCDebug(lcInput).noquote() << "key" << QKeySequence(key->keyCombination()).toString()
                                       << (key->isAutoRepeat() ? "(repeat)" : "")
                                       << "-> focus:" << describe(m_window->activeFocusItem())
                                       << "| window active:" << m_window->isActive();
        }

        return false;
    }

private:
    QQuickWindow *m_window;
};
}

void InputDebug::install(const QQmlApplicationEngine *engine)
{
    if (!lcInput().isDebugEnabled() || engine->rootObjects().isEmpty()) {
        return;
    }

    auto *window = qobject_cast<QQuickWindow *>(engine->rootObjects().constFirst());
    if (!window) {
        return;
    }

    window->installEventFilter(new KeyLogger(window));
    QObject::connect(window, &QQuickWindow::activeFocusItemChanged, window, [window] {
        qCDebug(lcInput).noquote() << "focus ->" << describe(window->activeFocusItem());
    });

    QObject::connect(window, &QWindow::activeChanged, window, [window] {
        qCDebug(lcInput) << "window active:" << window->isActive();
    });

    QObject::connect(qApp,
                     &QGuiApplication::applicationStateChanged,
                     window,
                     [](Qt::ApplicationState state) {
                         qCDebug(lcInput) << "application state:" << state;
                     });

    // A blocked event loop looks like ignored keys, so report it separately.
    auto *stallTimer = new QTimer(window);
    auto *clock = new QElapsedTimer;
    clock->start();

    QObject::connect(stallTimer, &QTimer::timeout, window, [clock] {
        const qint64 elapsed = clock->restart();
        if (elapsed > s_stallReportMs) {
            qCDebug(lcInput) << "event loop was blocked for" << elapsed << "ms";
        }
    });

    QObject::connect(stallTimer, &QObject::destroyed, [clock]() {
        delete clock;
    });

    stallTimer->start(s_stallCheckMs);
    qCDebug(lcInput).noquote() << "input debugging enabled, focus:"
                               << describe(window->activeFocusItem());
}
