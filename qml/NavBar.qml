// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import Flimmer

// Top navigation. Moving left/right switches pages immediately; down/OK enters the page.
BigscreenUI.NavigationBar {
    id: root
    property string currentPage: "channels"
    signal pageSelected(string page)
    signal enterPage

    readonly property var pages: ["channels", "guide", "settings"]

    currentIndex: Math.max(0, pages.indexOf(currentPage))
    SafeArea.additionalMargins.left: Theme.margin
    SafeArea.additionalMargins.right: Theme.margin

    actions: [
        Kirigami.Action {
            text: qsTr("Live TV")
            icon.name: "video-television"
        },
        Kirigami.Action {
            text: qsTr("TV Guide")
            icon.name: "view-calendar-timeline"
        },
        Kirigami.Action {
            text: qsTr("Settings")
            icon.name: "configure"
        }
    ]

    onActivated: index => {
        pageSelected(pages[index]);
        enterPage();
    }

    Keys.onDownPressed: enterPage()

    // Focusing a tab already shows its page.
    Connections {
        target: root.Window.window
        function onActiveFocusItemChanged() {
            const item = root.Window.activeFocusItem;

            if (!root.activeFocus || !(item instanceof T.AbstractButton))
                return;

            for (let i = 0; i < root.actions.length; ++i) {
                if (root.actions[i] === item.action && i !== root.currentIndex)
                    root.pageSelected(root.pages[i]);
            }
        }
    }

    leading: Row {
        spacing: 11

        Image {
            anchors.verticalCenter: parent.verticalCenter
            width: 32
            height: 32
            source: Qt.resolvedUrl("../data/icons/hicolor/256x256/apps/flimmer.png")
            sourceSize: Qt.size(96, 96)
            smooth: true
        }
        BigscreenUI.Label {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Live TV")
            type: BigscreenUI.Label.Heading
            font.weight: Font.DemiBold
        }
    }

    trailing: Row {
        spacing: 19

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            visible: Tvh.status !== Tvh.Online || Tvh.busy
            BigscreenUI.BusyIndicator {
                width: 21
                height: 21
                running: visible
                anchors.verticalCenter: parent.verticalCenter
                visible: Tvh.busy || Tvh.status === Tvh.Connecting
            }
            Rectangle {
                width: 8
                height: 8
                radius: 4
                anchors.verticalCenter: parent.verticalCenter
                visible: !Tvh.busy && Tvh.status !== Tvh.Connecting
                color: Tvh.status === Tvh.Offline ? Theme.danger : Theme.warning
            }
            BigscreenUI.Label {
                anchors.verticalCenter: parent.verticalCenter
                text: Tvh.busy && Tvh.status === Tvh.Online ? qsTr("Updating guide…") : Tvh.statusText
                color: Theme.textDim
                elide: Text.ElideRight
                width: Math.min(implicitWidth, 280)
            }
        }

        BigscreenUI.Label {
            id: clock
            anchors.verticalCenter: parent.verticalCenter
            type: BigscreenUI.Label.Section
            text: Theme.formatTime(Date.now())
            Timer {
                interval: 1000
                running: true
                repeat: true
                onTriggered: clock.text = Theme.formatTime(Date.now())
            }
        }
    }
}
