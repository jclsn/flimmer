// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick

Rectangle {
    id: root
    property real value: 0
    property color fillColor: Theme.accent

    implicitHeight: 4
    radius: height / 2
    color: Theme.outline

    Rectangle {
        width: Math.max(parent.height, parent.width * Math.min(1, Math.max(0, root.value)))
        height: parent.height
        radius: parent.radius
        color: root.fillColor
        visible: root.value > 0
        Behavior on width {
            NumberAnimation {
                duration: Theme.animNormal
            }
        }
    }
}
