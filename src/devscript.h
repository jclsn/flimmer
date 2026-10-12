#ifndef DEVSCRIPT_H
#define DEVSCRIPT_H

class QQmlApplicationEngine;

namespace DevScript
{
// Development aid (built with -DPLASMA_TV_DEV_TOOLS=ON): runs the script named by
// $PLASMA_TV_SCRIPT, one command per line: "wait <ms>", "key <QKeySequence name>",
// "text <chars>", "shot <file.png>", "focusout"/"focusin" (move focus to another
// window and back), "quit".
void runFromEnvironment(QQmlApplicationEngine *engine);
}

#endif // DEVSCRIPT_H
