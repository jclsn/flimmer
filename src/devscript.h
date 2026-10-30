/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DEVSCRIPT_H
#define DEVSCRIPT_H

class QQmlApplicationEngine;

namespace DevScript
{
// Development aid (built with -DFLIMMER_DEV_TOOLS=ON): runs the script named by
// $FLIMMER_SCRIPT, one command per line: "wait <ms>", "key <QKeySequence name>",
// "text <chars>", "shot <file.png>", "focusout"/"focusin" (move focus to another
// window of this application and back), "appinactive"/"appactive" (as if
// another application took the focus and gave it back), "quit".
void runFromEnvironment(QQmlApplicationEngine *engine);
}

#endif // DEVSCRIPT_H
