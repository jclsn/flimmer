// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import org.kde.bigscreen.ui as BigscreenUI

// Bigscreen button that can be unavailable without losing the remote focus.
BigscreenUI.Button {
    id: root
    // Not Item.enabled: a disabled item would drop the remote focus.
    property bool available: true
    signal activated

    opacity: available ? 1 : 0.4
    onClicked: {
        if (root.available)
            root.activated();
    }
}
