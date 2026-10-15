/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SCREENSAVERINHIBITOR_H
#define SCREENSAVERINHIBITOR_H

#include <QObject>
#include <qqmlregistration.h>

// Keeps the screen awake while 'active' is true, using the freedesktop
// ScreenSaver D-Bus interface (implemented by KDE Plasma, GNOME and others).
class ScreenSaverInhibitor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    explicit ScreenSaverInhibitor(QObject *parent = nullptr);
    ~ScreenSaverInhibitor() override;

    bool active() const
    {
        return m_active;
    }
    void setActive(bool active);

Q_SIGNALS:
    void activeChanged();

private:
    void inhibit();
    void uninhibit();

    bool m_active = false;
    uint m_cookie = 0;
};

#endif // SCREENSAVERINHIBITOR_H
