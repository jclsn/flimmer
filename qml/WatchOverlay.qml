// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import PlasmaTv

// On-screen display while watching: info bar, zapping, channel list overlay.
FocusScope {
    id: root
    required property Item player
    signal leave(string page)

    // OSD sits on top of video, so use Plasma's complementary (dark) colors.
    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false

    // Channel chosen with up/down whose stream hasn't been started yet.
    property string pendingUuid
    readonly property string shownUuid: pendingUuid.length > 0 ? pendingUuid : player.channelUuid
    readonly property int channelIndex: {
        Tvh.channels.count;
        return Tvh.channels.indexOfUuid(shownUuid);
    }
    readonly property var channel: {
        Tvh.now;
        Tvh.epgRevision;
        return Tvh.channels.get(channelIndex);
    }
    readonly property var nowEvent: {
        Tvh.now;
        Tvh.epgRevision;
        return Tvh.eventAt(shownUuid, Date.now());
    }
    readonly property var nextEvent: {
        Tvh.now;
        Tvh.epgRevision;
        return Tvh.nextEvent(shownUuid, nowEvent.start ?? Date.now());
    }
    property bool infoShown: false

    function play(uuid) {
        zapDelay.stop();
        pendingUuid = "";
        player.play(uuid);
        Settings.lastChannel = uuid;
        showInfo();
    }

    // Up/down only update the info bar; the stream starts once the user stops
    // pressing, so zapping through channels doesn't open a stream for each one.
    function zap(delta) {
        const n = Tvh.channels.count;
        if (n === 0)
            return;
        const i = channelIndex < 0 ? 0 : (channelIndex + delta + n) % n;
        pendingUuid = Tvh.channels.get(i).uuid;
        zapDelay.restart();
        showInfo();
    }

    function showInfo() {
        infoShown = true;
        infoTimer.restart();
    }

    function togglePause() {
        player.togglePause();
        showInfo();
    }

    function flash(text) {
        feedback.show(text, "");
        showInfo();
    }

    function openZapList() {
        zapList.currentIndex = Math.max(0, channelIndex);
        zapList.positionViewAtIndex(zapList.currentIndex, ListView.Center);
        zapPanel.shown = true;
        zapList.forceActiveFocus();
        zapTimer.restart();
    }

    function closeZapList() {
        zapTimer.stop();
        zapPanel.shown = false;
        // The overlay is a focus scope that remembers the list as its focused
        // child; without clearing that, focusing the overlay would hand the keys
        // straight back to the hidden list.
        zapList.focus = false;
        // Only reclaim focus while watching; the overlay may already be hidden.
        if (root.visible)
            root.forceActiveFocus();
    }

    onVisibleChanged: {
        if (!visible) {
            zapTimer.stop();
            zapPanel.shown = false;
            zapList.focus = false;
        }
    }

    Keys.onPressed: event => {
        switch (event.key) {
        case Qt.Key_Up:
        case Qt.Key_ChannelUp:
        case Qt.Key_PageUp:
            zap(1);
            break;
        case Qt.Key_Down:
        case Qt.Key_ChannelDown:
        case Qt.Key_PageDown:
            zap(-1);
            break;
        case Qt.Key_Left:
            openZapList();
            break;
        case Qt.Key_Right:
            leave("guide");
            break;
        case Qt.Key_Info:
        case Qt.Key_I:
            if (infoShown)
                infoShown = false;
            else
                showInfo();
            break;
        case Qt.Key_AudioCycleTrack:
        case Qt.Key_A:
            flash(player.cycleAudio());
            break;
        case Qt.Key_Subtitle:
        case Qt.Key_S:
            flash(player.cycleSubtitles());
            break;
        case Qt.Key_MediaPlay:
        case Qt.Key_MediaPause:
        case Qt.Key_MediaTogglePlayPause:
        case Qt.Key_P:
            togglePause();
            break;
        case Qt.Key_MediaStop:
        case Qt.Key_X:
            player.stop();
            leave("channels");
            break;
        default:
            if (Theme.isAccept(event.key) && !event.isAutoRepeat) {
                if (player.error.length > 0)
                    player.retry();
                else
                    togglePause();
            } else if (Theme.isBack(event.key)) {
                quitMenu.open();
            } else {
                return;
            }
        }
        event.accepted = true;
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.infoShown ? (root.infoShown = false) : root.showInfo()
        onDoubleClicked: root.leave("channels")
    }

    Timer {
        id: infoTimer
        interval: 6000
        onTriggered: root.infoShown = false
    }
    Timer {
        id: zapTimer
        interval: 15000
        onTriggered: root.closeZapList()
    }
    Timer {
        id: zapDelay
        interval: 450
        onTriggered: {
            if (root.pendingUuid === root.player.channelUuid && root.player.error.length === 0) {
                root.pendingUuid = "";
            } else {
                root.play(root.pendingUuid);
            }
        }
    }

    // --- Error ------------------------------------------------------------
    Rectangle {
        anchors.centerIn: parent
        visible: root.player.error.length > 0 && !zapPanel.shown
        width: 667
        height: errorColumn.implicitHeight + 53
        radius: Theme.radius + 5
        color: Kirigami.Theme.backgroundColor
        opacity: 0.95

        BigscreenUI.PlaceholderMessage {
            id: errorColumn
            anchors.centerIn: parent
            width: parent.width - 53
            iconName: "dialog-error"
            text: qsTr("Playback problem")
            explanation: root.player.error + "\n\n" + qsTr("Press OK to retry, BACK to return")
        }
    }

    // --- Short feedback (audio track, …) ------------------------------------
    BigscreenUI.Toast {
        id: feedback
        anchors.bottom: undefined
        anchors.top: parent.top
        anchors.topMargin: 43
        timeout: 3000
    }

    Connections {
        target: root.player
        function onNotice(text) {
            root.flash(text);
        }
    }

    // --- BACK: quit or leave -------------------------------------------------
    BigscreenUI.Dialog {
        id: quitMenu
        title: qsTr("Quit Plasma TV?")
        openFocusItem: quitButton

        onAboutToHide: {
            if (root.visible)
                root.forceActiveFocus();
        }

        contentItem: RowLayout {
            spacing: 11
            BigscreenUI.KeyNavigationGroup.orientation: Qt.Horizontal

            TvButton {
                id: quitButton
                text: qsTr("Quit")
                icon.name: "application-exit"
                onActivated: {
                    // Also deletes the recording made while paused.
                    root.player.stop();
                    Qt.quit();
                }
            }
            TvButton {
                text: qsTr("Live TV")
                icon.name: "video-television"
                onActivated: {
                    quitMenu.close();
                    root.leave("channels");
                }
            }
            TvButton {
                text: qsTr("Keep watching")
                icon.name: "media-playback-start"
                onActivated: quitMenu.close()
            }
        }
    }

    // --- Paused / behind live ------------------------------------------------
    BigscreenUI.Badge {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.margin
        visible: root.player.paused || root.player.timeshifting
        type: BigscreenUI.Label.Heading
        text: {
            const minutes = Math.floor(root.player.behindLive / 60000);

            if (root.player.paused)
                return qsTr("❚❚  Paused");

            return minutes < 1 ? qsTr("Behind live") : qsTr("%1 min behind live").arg(minutes);
        }
    }

    // --- Info bar ---------------------------------------------------------
    Item {
        id: info
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 253
        opacity: root.infoShown && !zapPanel.shown ? 1 : 0
        visible: opacity > 0
        Behavior on opacity {
            NumberAnimation {
                duration: Theme.animNormal
            }
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: "transparent"
                }
                GradientStop {
                    position: 0.45
                    color: Qt.rgba(0, 0, 0, 0.7)
                }
                GradientStop {
                    position: 1
                    color: Qt.rgba(0, 0, 0, 0.9)
                }
            }
        }

        ChannelLogo {
            id: osdLogo
            anchors.left: parent.left
            anchors.leftMargin: Theme.margin
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 48
            width: 147
            height: 88
            source: root.channel.iconUrl ?? ""
            name: root.channel.name ?? ""
        }

        Column {
            anchors.left: osdLogo.right
            anchors.leftMargin: 27
            anchors.right: clock.left
            anchors.rightMargin: 27
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 43
            spacing: 7

            Text {
                text: (root.channel.number > 0 ? root.channel.number + "  " : "") + (root.channel.name ?? "")
                color: Kirigami.Theme.highlightColor
                font.pixelSize: Theme.fontNormal
                font.weight: Font.DemiBold
            }
            Text {
                width: parent.width
                text: root.nowEvent.title ?? qsTr("No programme information")
                color: "white"
                font.pixelSize: Theme.fontTitle
                font.weight: Font.Bold
                elide: Text.ElideRight
            }
            Row {
                spacing: 16
                width: parent.width
                visible: root.nowEvent.start !== undefined
                Text {
                    id: rangeText
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.nowEvent.start !== undefined ? Theme.formatRange(root.nowEvent.start, root.nowEvent.stop) : ""
                    color: "#d0d6df"
                    font.pixelSize: Theme.fontSmall
                }
                BigscreenUI.ProgressBar {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - rangeText.width - leftText.width - 32
                    value: root.nowEvent.start !== undefined ? (Tvh.now - root.nowEvent.start) / (root.nowEvent.stop - root.nowEvent.start) : 0
                }
                Text {
                    id: leftText
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.nowEvent.stop !== undefined ? qsTr("%1 min left").arg(Theme.minutesLeft(root.nowEvent.stop, Tvh.now)) : ""
                    color: "#d0d6df"
                    font.pixelSize: Theme.fontSmall
                }
            }
            Text {
                width: parent.width
                visible: root.nextEvent.title !== undefined
                text: qsTr("Next  %1  %2").arg(Theme.formatTime(root.nextEvent.start ?? 0)).arg(root.nextEvent.title ?? "")
                color: "#aab3c0"
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }
        }

        Text {
            id: clock
            anchors.right: parent.right
            anchors.rightMargin: Theme.margin
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 48
            text: Theme.formatTime(Tvh.now)
            color: "white"
            font.pixelSize: Theme.fontHuge
            font.weight: Font.Light
        }
    }

    // --- Channel list overlay ----------------------------------------------
    Rectangle {
        id: zapPanel
        property bool shown: false
        width: 507
        height: parent.height
        x: shown ? 0 : -width
        // Visible as soon as it opens (before the slide-in has moved it), so
        // the list can take the keyboard focus right away.
        visible: shown || x > -width
        color: Kirigami.Theme.backgroundColor
        Behavior on x {
            NumberAnimation {
                duration: Theme.animNormal
                easing.type: Easing.OutCubic
            }
        }

        Text {
            id: zapTitle
            x: 32
            y: 32
            text: qsTr("Channels")
            color: Kirigami.Theme.textColor
            font.pixelSize: Theme.fontLarge
        }

        ListView {
            id: zapList
            anchors.top: zapTitle.bottom
            anchors.topMargin: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 21
            x: 16
            width: parent.width - 32
            clip: true
            model: Tvh.channels
            spacing: 5
            keyNavigationEnabled: true
            highlightMoveDuration: 120
            highlightRangeMode: ListView.ApplyRange
            preferredHighlightBegin: height * 0.3
            preferredHighlightEnd: height * 0.6

            Keys.onPressed: event => {
                zapTimer.restart();
                if (Theme.isAccept(event.key) && !event.isAutoRepeat) {
                    root.play(Tvh.channels.get(currentIndex).uuid);
                    root.closeZapList();
                } else if (Theme.isBack(event.key) || event.key === Qt.Key_Left) {
                    root.closeZapList();
                } else if (event.key === Qt.Key_PageUp || event.key === Qt.Key_ChannelUp) {
                    currentIndex = Math.max(0, currentIndex - 8);
                } else if (event.key === Qt.Key_PageDown || event.key === Qt.Key_ChannelDown) {
                    currentIndex = Math.min(count - 1, currentIndex + 8);
                } else if (event.key === Qt.Key_Right) {
                    // swallow
                } else {
                    return;
                }
                event.accepted = true;
            }

            delegate: Rectangle {
                id: zapRow
                required property int index
                required property string uuid
                required property int number
                required property string name
                required property url iconUrl
                required property string nowTitle
                required property double nowProgress
                readonly property bool current: ListView.isCurrentItem
                readonly property bool playing: uuid === root.player.channelUuid

                width: ListView.view.width
                height: 67
                radius: Theme.radiusSmall
                color: current ? Kirigami.Theme.highlightColor : "transparent"

                Text {
                    id: zapNumber
                    anchors.left: parent.left
                    anchors.leftMargin: 11
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40
                    horizontalAlignment: Text.AlignRight
                    text: zapRow.number > 0 ? zapRow.number : ""
                    color: zapRow.current ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.disabledTextColor
                    font.pixelSize: Theme.fontSmall
                }
                ChannelLogo {
                    id: zapLogo
                    anchors.left: zapNumber.right
                    anchors.leftMargin: 13
                    anchors.verticalCenter: parent.verticalCenter
                    width: 73
                    height: 44
                    source: zapRow.iconUrl
                    name: zapRow.name
                }
                Column {
                    anchors.left: zapLogo.right
                    anchors.leftMargin: 13
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    Row {
                        spacing: 8
                        width: parent.width
                        Kirigami.Icon {
                            visible: zapRow.playing
                            width: 17
                            height: 17
                            anchors.verticalCenter: parent.verticalCenter
                            source: "media-playback-start"
                            isMask: true
                            color: zapRow.current ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.highlightColor
                        }
                        Text {
                            width: parent.width - (zapRow.playing ? 25 : 0)
                            text: zapRow.name
                            color: zapRow.current ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                            font.pixelSize: Theme.fontNormal
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                    }
                    Text {
                        width: parent.width
                        text: zapRow.nowTitle
                        color: zapRow.current ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.disabledTextColor
                        font.pixelSize: Theme.fontSmall
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.play(zapRow.uuid);
                        root.closeZapList();
                    }
                }
            }
        }
    }
}
