#include "devscript.h"

#include <QCoreApplication>
#include <QFile>
#include <QKeyEvent>
#include <QKeySequence>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

#include <memory>

namespace
{
struct Runner {
    QStringList lines;
    int position = 0;
    QQuickWindow *window = nullptr;

    void sendKey(int key, Qt::KeyboardModifiers mods, const QString &text)
    {
        QKeyEvent press(QEvent::KeyPress, key, mods, text);
        QCoreApplication::sendEvent(window, &press);
        QKeyEvent release(QEvent::KeyRelease, key, mods, text);
        QCoreApplication::sendEvent(window, &release);
    }

    void step(const std::shared_ptr<Runner> &self)
    {
        int delay = 50;
        while (position < lines.size()) {
            const QString line = lines.at(position++).trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
                continue;
            }

            const QString command = line.section(QLatin1Char(' '), 0, 0);
            const QString arg = line.section(QLatin1Char(' '), 1);
            if (command == QLatin1String("wait")) {
                delay = arg.toInt();
                break;
            } else if (command == QLatin1String("key")) {
                const QKeyCombination combo = QKeySequence::fromString(arg)[0];
                const int key = combo.key();
                const QString text = key >= Qt::Key_Space && key <= Qt::Key_AsciiTilde
                    ? QString(QChar(key)).toLower()
                    : QString();
                sendKey(key, combo.keyboardModifiers(), text);
                delay = 150;
                break;
            } else if (command == QLatin1String("text")) {
                for (const QChar character : arg) {
                    sendKey(QKeySequence(QString(character))[0].key(),
                            Qt::NoModifier,
                            QString(character));
                }
            } else if (command == QLatin1String("shot")) {
                window->grabWindow().save(arg);
                qInfo() << "screenshot saved:" << arg;
            } else if (command == QLatin1String("quit")) {
                QCoreApplication::quit();
                return;
            }
        }

        if (position < lines.size()) {
            QTimer::singleShot(delay, window, [self]() {
                self->step(self);
            });
        }
    }
};
}

void DevScript::runFromEnvironment(QQmlApplicationEngine *engine)
{
    const QString path = qEnvironmentVariable("PLASMA_TV_SCRIPT");
    if (path.isEmpty() || engine->rootObjects().isEmpty()) {
        return;
    }

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    auto runner = std::make_shared<Runner>();
    runner->lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    runner->window = qobject_cast<QQuickWindow *>(engine->rootObjects().constFirst());

    if (!runner->window) {
        return;
    }

    QTimer::singleShot(0, runner->window, [runner]() {
        runner->step(runner);
    });
}
