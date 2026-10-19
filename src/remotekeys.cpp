// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "remotekeys.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QWindow>

namespace
{
class OkKeyFilter : public QObject
{
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease) {
            return false;
        }

        // Only the event sent to the window; Qt Quick passes it on to the items.
        if (!qobject_cast<QWindow *>(watched)) {
            return false;
        }

        auto *key = static_cast<QKeyEvent *>(event);

        if (key->key() != Qt::Key_Select && key->key() != Qt::Key_Enter) {
            return false;
        }

        QKeyEvent ret(key->type(),
                      Qt::Key_Return,
                      key->modifiers() & ~Qt::KeypadModifier,
                      QStringLiteral("\r"),
                      key->isAutoRepeat(),
                      key->count());
        QCoreApplication::sendEvent(watched, &ret);
        event->setAccepted(ret.isAccepted());

        return true;
    }
};
}

void RemoteKeys::install(QCoreApplication *app)
{
    app->installEventFilter(new OkKeyFilter(app));
}
