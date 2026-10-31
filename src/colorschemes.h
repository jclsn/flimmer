/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef COLORSCHEMES_H
#define COLORSCHEMES_H

#include <QString>
#include <QVariantList>

// Plasma color scheme support without linking KColorScheme: the scheme file
// is announced through the KDE_COLOR_SCHEME_PATH application property (read by
// KColorScheme, Kirigami's Plasma theme and plasma-integration) and the
// application palette is built from it so Qt Quick Controls match.
namespace ColorSchemes
{
// Default scheme: Breeze Dark suits a TV in a dim living room.
// QStringLiteral avoids heap allocation; construction is effectively non-throwing here
inline const QString s_default = // NOLINT -cppcoreguidelines-avoid-non-const-global-variables
    QStringLiteral("BreezeDark");

// [{ id: "", name: "Plasma system colors" }, { id: "BreezeDark", name: "Breeze Dark" }, ...]
QVariantList available();
// Empty id follows the system (Plasma) color scheme.
void apply(const QString &id);
}

#endif // COLORSCHEMES_H
