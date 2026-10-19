/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef REMOTEKEYS_H
#define REMOTEKEYS_H

class QCoreApplication;

namespace RemoteKeys
{
// Remotes send OK as Select (CEC) or Enter (keypad). Bigscreen UI controls only
// react to Return, so both are turned into Return before they reach a window.
void install(QCoreApplication *app);
}

#endif // REMOTEKEYS_H
