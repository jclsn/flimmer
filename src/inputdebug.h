/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef INPUTDEBUG_H
#define INPUTDEBUG_H

class QQmlApplicationEngine;

namespace InputDebug
{
// Logs key presses, keyboard focus changes, application state changes and
// stalls of the event loop to help with "the remote stops working" reports.
// Enabled with QT_LOGGING_RULES="flimmer.input.debug=true".
void install(QQmlApplicationEngine *engine);
}

#endif // INPUTDEBUG_H
