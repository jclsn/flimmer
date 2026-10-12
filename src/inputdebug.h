#ifndef INPUTDEBUG_H
#define INPUTDEBUG_H

class QQmlApplicationEngine;

namespace InputDebug
{
// Logs key presses, keyboard focus changes, application state changes and
// stalls of the event loop to help with "the remote stops working" reports.
// Enabled with QT_LOGGING_RULES="plasmatv.input.debug=true".
void install(QQmlApplicationEngine *engine);
}

#endif // INPUTDEBUG_H
