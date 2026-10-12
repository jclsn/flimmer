#include "colorschemes.h"

#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>

namespace
{
QString schemePath(const QString &id)
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                  QStringLiteral("color-schemes/%1.colors").arg(id));
}

QColor readColor(const QSettings &scheme, const QString &group, const QString &key)
{
    const QVariant value = scheme.value(QStringLiteral("Colors:%1/%2").arg(group, key));
    // QSettings splits "r,g,b" into a string list.
    const QStringList parts = value.typeId() == QMetaType::QStringList
        ? value.toStringList()
        : value.toString().split(QLatin1Char(','));
    if (parts.size() < 3) {
        return {};
    }

    return QColor(parts.at(0).trimmed().toInt(),
                  parts.at(1).trimmed().toInt(),
                  parts.at(2).trimmed().toInt());
}

QPalette paletteFromScheme(const QString &path)
{
    const QSettings scheme(path, QSettings::IniFormat);
    QPalette palette;

    auto set = [&palette](QPalette::ColorRole role, const QColor &color) {
        if (color.isValid()) {
            palette.setColor(role, color);
        }
    };
    set(QPalette::Window,
        readColor(scheme, QStringLiteral("Window"), QStringLiteral("BackgroundNormal")));
    set(QPalette::WindowText,
        readColor(scheme, QStringLiteral("Window"), QStringLiteral("ForegroundNormal")));
    set(QPalette::Base,
        readColor(scheme, QStringLiteral("View"), QStringLiteral("BackgroundNormal")));
    set(QPalette::AlternateBase,
        readColor(scheme, QStringLiteral("View"), QStringLiteral("BackgroundAlternate")));
    set(QPalette::Text,
        readColor(scheme, QStringLiteral("View"), QStringLiteral("ForegroundNormal")));
    set(QPalette::Button,
        readColor(scheme, QStringLiteral("Button"), QStringLiteral("BackgroundNormal")));
    set(QPalette::ButtonText,
        readColor(scheme, QStringLiteral("Button"), QStringLiteral("ForegroundNormal")));
    set(QPalette::Highlight,
        readColor(scheme, QStringLiteral("Selection"), QStringLiteral("BackgroundNormal")));
    set(QPalette::HighlightedText,
        readColor(scheme, QStringLiteral("Selection"), QStringLiteral("ForegroundNormal")));
    set(QPalette::Accent,
        readColor(scheme, QStringLiteral("Selection"), QStringLiteral("BackgroundNormal")));
    set(QPalette::ToolTipBase,
        readColor(scheme, QStringLiteral("Tooltip"), QStringLiteral("BackgroundNormal")));
    set(QPalette::ToolTipText,
        readColor(scheme, QStringLiteral("Tooltip"), QStringLiteral("ForegroundNormal")));
    set(QPalette::Link,
        readColor(scheme, QStringLiteral("View"), QStringLiteral("ForegroundLink")));
    set(QPalette::LinkVisited,
        readColor(scheme, QStringLiteral("View"), QStringLiteral("ForegroundVisited")));
    set(QPalette::PlaceholderText,
        readColor(scheme, QStringLiteral("View"), QStringLiteral("ForegroundInactive")));

    const QColor button = palette.color(QPalette::Button);
    palette.setColor(QPalette::Light, button.lighter(150));
    palette.setColor(QPalette::Midlight, button.lighter(125));
    palette.setColor(QPalette::Mid, button.darker(150));
    palette.setColor(QPalette::Dark, button.darker(200));
    palette.setColor(QPalette::Shadow, QColor(0, 0, 0));

    const QColor inactiveText =
        readColor(scheme, QStringLiteral("View"), QStringLiteral("ForegroundInactive"));
    if (inactiveText.isValid()) {
        for (QPalette::ColorRole role :
             {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
            palette.setColor(QPalette::Disabled, role, inactiveText);
        }
    }

    return palette;
}
}

QVariantList ColorSchemes::available()
{
    QVariantList result;
    result.append(
        QVariantMap{{QStringLiteral("id"), QString()},
                    {QStringLiteral("name"),
                     QCoreApplication::translate("ColorSchemes", "Plasma system colors")}});

    QMap<QString, QString> schemes; // id -> display name, sorted by id
    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                       QStringLiteral("color-schemes"),
                                                       QStandardPaths::LocateDirectory);
    for (const QString &dir : dirs) {
        const QStringList files = QDir(dir).entryList({QStringLiteral("*.colors")}, QDir::Files);
        for (const QString &file : files) {
            const QString id = file.chopped(7);
            if (schemes.contains(id)) {
                continue; // user-local copies take precedence (locateAll returns them first)
            }

            // QSettings maps the [General] section of INI files to the top level.
            const QSettings scheme(dir + QLatin1Char('/') + file, QSettings::IniFormat);
            schemes.insert(id, scheme.value(QStringLiteral("Name"), id).toString());
        }
    }

    QList<QPair<QString, QString>> sorted;
    for (auto it = schemes.cbegin(); it != schemes.cend(); ++it) {
        sorted.append({it.key(), it.value()});
    }

    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
        return a.second.localeAwareCompare(b.second) < 0;
    });

    for (const auto &[id, name] : sorted) {
        result.append(QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("name"), name}});
    }

    return result;
}

void ColorSchemes::apply(const QString &id)
{
    const QString path = id.isEmpty() ? QString() : schemePath(id);
    if (path.isEmpty()) {
        qApp->setProperty("KDE_COLOR_SCHEME_PATH", QVariant());
        QGuiApplication::setPalette(QPalette()); // back to the platform (Plasma) palette
        return;
    }

    qApp->setProperty("KDE_COLOR_SCHEME_PATH", path);
    // Changing the palette makes Kirigami re-read the scheme.
    QGuiApplication::setPalette(paletteFromScheme(path));
}
