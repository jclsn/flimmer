// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick

// Channel logo on a soft plate; falls back to the channel's initials.
Rectangle {
    id: root
    property url source
    property string name

    radius: Theme.radiusSmall
    color: Theme.surfaceRaised
    border.color: Theme.outline
    border.width: 1

    Image {
        id: image
        anchors.fill: parent
        anchors.margins: parent.height * 0.12
        // Logos go through the caching provider (see LogoImageProvider).
        source: root.source.toString().length > 0 ? "image://logo/" + encodeURIComponent(root.source.toString()) : ""
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        smooth: true
        mipmap: true
        sourceSize.height: 256
        opacity: status === Image.Ready ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: Theme.animNormal
            }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: image.status !== Image.Ready
        text: {
            const words = root.name.trim().split(/\s+/).filter(w => w.length > 0);
            if (words.length === 0)
                return "TV";
            if (words.length === 1)
                return words[0].substring(0, 3).toUpperCase();
            return (words[0][0] + words[1][0]).toUpperCase();
        }
        color: Theme.textDim
        font.pixelSize: root.height * 0.32
        font.weight: Font.DemiBold
    }
}
