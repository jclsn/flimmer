import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import PlasmaTv

// Live TV: channel list on the left, details of the highlighted channel on the right.
FocusScope {
    id: root
    signal exitUp
    signal watch(string uuid)
    signal openGuide(string uuid)

    property alias currentIndex: list.currentIndex
    readonly property bool empty: Tvh.channels.count === 0

    function selectIndex(index) {
        if (index >= 0 && index < list.count)
            list.positionViewAtIndex(index, ListView.Center);
        list.currentIndex = index;
    }

    readonly property var current: {
        Tvh.now;
        Tvh.epgRevision;  // re-evaluate as time passes or data changes
        return Tvh.channels.get(list.currentIndex);
    }
    readonly property var upcoming: {
        Tvh.now;
        Tvh.epgRevision;
        if (!current.uuid)
            return [];
        return Tvh.eventsForChannel(current.uuid, Date.now(), Date.now() + 12 * 3600 * 1000).slice(1, 4);
    }

    ListView {
        id: list
        x: Theme.margin - 16
        width: 960
        height: parent.height
        topMargin: 16
        bottomMargin: 48
        leftMargin: 16
        rightMargin: 16
        model: Tvh.channels
        focus: true
        clip: true
        spacing: 14
        visible: !root.empty
        keyNavigationEnabled: true
        highlightMoveDuration: 160
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: height * 0.3
        preferredHighlightEnd: height * 0.62
        cacheBuffer: 1200

        // Channels may arrive after the page is shown (first start, no cache yet).
        onCountChanged: {
            if (currentIndex < 0 && count > 0)
                root.selectIndex(Math.max(0, Tvh.channels.indexOfUuid(Settings.lastChannel)));
        }

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Up && currentIndex <= 0) {
                root.exitUp();
            } else if (event.key === Qt.Key_PageUp || event.key === Qt.Key_ChannelUp) {
                currentIndex = Math.max(0, currentIndex - 6);
            } else if (event.key === Qt.Key_PageDown || event.key === Qt.Key_ChannelDown) {
                currentIndex = Math.min(count - 1, currentIndex + 6);
            } else if (event.key === Qt.Key_Right) {
                if (root.current.uuid)
                    root.openGuide(root.current.uuid);
            } else if (Theme.isAccept(event.key) && !event.isAutoRepeat) {
                if (root.current.uuid)
                    root.watch(root.current.uuid);
            } else {
                return;
            }
            event.accepted = true;
        }

        delegate: Item {
            id: row
            required property int index
            required property string uuid
            required property int number
            required property string name
            required property url iconUrl
            required property string nowTitle
            required property double nowStop
            required property double nowProgress
            required property int nowGenre
            required property bool nowRecording
            readonly property bool focused: ListView.isCurrentItem && list.activeFocus

            width: ListView.view.width - 32
            height: 116
            z: focused ? 2 : 1

            Rectangle {
                id: card
                anchors.fill: parent
                radius: Theme.radius
                color: row.focused ? Theme.surfaceHover : (row.ListView.isCurrentItem ? Theme.surfaceRaised : Theme.surface)
                border.width: row.focused ? 3 : 0
                border.color: Theme.focusRing
                scale: row.focused ? 1.03 : 1
                Behavior on scale {
                    NumberAnimation {
                        duration: Theme.animFast
                    }
                }
                Behavior on color {
                    ColorAnimation {
                        duration: Theme.animFast
                    }
                }

                Rectangle {
                    width: 6
                    radius: 3
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height - 40
                    color: Theme.genreColor(row.nowGenre)
                    visible: row.nowTitle.length > 0
                }

                Text {
                    id: numberLabel
                    anchors.left: parent.left
                    anchors.leftMargin: 24
                    anchors.verticalCenter: parent.verticalCenter
                    width: 72
                    horizontalAlignment: Text.AlignRight
                    text: row.number > 0 ? row.number : ""
                    color: row.focused ? Theme.text : Theme.textFaint
                    font.pixelSize: Theme.fontLarge
                    font.weight: Font.Light
                }

                ChannelLogo {
                    id: logo
                    anchors.left: numberLabel.right
                    anchors.leftMargin: 24
                    anchors.verticalCenter: parent.verticalCenter
                    width: 132
                    height: 80
                    source: row.iconUrl
                    name: row.name
                }

                Column {
                    anchors.left: logo.right
                    anchors.leftMargin: 28
                    anchors.right: remaining.left
                    anchors.rightMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    Text {
                        width: parent.width
                        text: row.name
                        color: Theme.text
                        font.pixelSize: Theme.fontNormal
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Row {
                        width: parent.width
                        spacing: 10
                        Rectangle {
                            visible: row.nowRecording
                            width: 14
                            height: 14
                            radius: 7
                            color: Theme.recording
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            width: parent.width - (row.nowRecording ? 24 : 0)
                            text: row.nowTitle.length > 0 ? row.nowTitle : qsTr("No programme information")
                            color: row.nowTitle.length > 0 ? Theme.textDim : Theme.textFaint
                            font.pixelSize: Theme.fontSmall
                            elide: Text.ElideRight
                        }
                    }
                    ThinProgress {
                        width: parent.width
                        height: 5
                        value: row.nowProgress
                        visible: row.nowTitle.length > 0
                        fillColor: row.focused ? Theme.accent : Theme.textFaint
                    }
                }

                Text {
                    id: remaining
                    anchors.right: parent.right
                    anchors.rightMargin: 28
                    anchors.verticalCenter: parent.verticalCenter
                    visible: row.nowTitle.length > 0
                    text: qsTr("%1 min").arg(Theme.minutesLeft(row.nowStop, Tvh.now))
                    color: Theme.textFaint
                    font.pixelSize: Theme.fontSmall
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    list.forceActiveFocus();
                    if (list.currentIndex === row.index)
                        root.watch(row.uuid);
                    list.currentIndex = row.index;
                }
            }
        }
    }

    // Details of the highlighted channel
    Item {
        id: details
        visible: !root.empty && root.current.uuid !== undefined
        anchors.left: list.right
        anchors.leftMargin: 56
        anchors.right: parent.right
        anchors.rightMargin: Theme.margin
        anchors.top: parent.top
        anchors.topMargin: 16
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 48

        Rectangle {
            anchors.fill: parent
            radius: Theme.radius + 8
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: Theme.surface
                }
                GradientStop {
                    position: 1
                    color: Theme.card
                }
            }
            border.color: Theme.outline
        }

        Column {
            anchors.fill: parent
            anchors.margins: 48
            anchors.bottomMargin: 110   // room for the key hints
            spacing: 22
            clip: true

            Row {
                spacing: 32
                ChannelLogo {
                    width: 220
                    height: 132
                    source: root.current.iconUrl ?? ""
                    name: root.current.name ?? ""
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    Text {
                        text: root.current.number > 0 ? qsTr("Channel %1").arg(root.current.number) : ""
                        color: Theme.accent
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.DemiBold
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 2
                    }
                    Text {
                        width: details.width - 96 - 252
                        text: root.current.name ?? ""
                        color: Theme.text
                        font.pixelSize: Theme.fontTitle
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                }
            }

            Item {
                width: 1
                height: 8
            }

            Row {
                spacing: 16
                visible: (root.current.nowTitle ?? "").length > 0
                Rectangle {
                    width: nowLabel.implicitWidth + 28
                    height: 40
                    radius: 8
                    color: root.current.nowRecording ? Theme.recording : Theme.accent
                    Text {
                        id: nowLabel
                        anchors.centerIn: parent
                        text: root.current.nowRecording ? qsTr("● REC") : qsTr("NOW")
                        color: Theme.onAccent
                        font.pixelSize: Theme.fontTiny
                        font.weight: Font.Bold
                        font.letterSpacing: 1.5
                    }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.current.nowStart ? Theme.formatRange(root.current.nowStart, root.current.nowStop) + "  ·  " + qsTr("%1 min left").arg(Theme.minutesLeft(root.current.nowStop, Tvh.now)) : ""
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSmall
                }
            }

            Text {
                width: parent.width
                text: (root.current.nowTitle ?? "").length > 0 ? root.current.nowTitle : qsTr("No programme information available")
                color: Theme.text
                font.pixelSize: (root.current.nowTitle ?? "").length > 0 ? Theme.fontTitle : Theme.fontLarge
                font.weight: Font.Bold
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                visible: text.length > 0
                text: root.current.nowSubtitle ?? ""
                color: Theme.textDim
                font.pixelSize: Theme.fontNormal
                elide: Text.ElideRight
            }

            ThinProgress {
                width: parent.width
                height: 8
                value: root.current.nowProgress ?? 0
                visible: (root.current.nowTitle ?? "").length > 0
            }

            Text {
                width: parent.width
                visible: text.length > 0
                text: root.current.nowSummary ?? ""
                color: Theme.textDim
                font.pixelSize: Theme.fontSmall
                lineHeight: 1.2
                wrapMode: Text.Wrap
                maximumLineCount: 5
                elide: Text.ElideRight
            }

            Item {
                width: 1
                height: 8
            }

            Text {
                visible: root.upcoming.length > 0
                text: qsTr("Up next")
                color: Theme.accent
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 2
            }

            Repeater {
                model: root.upcoming
                delegate: Row {
                    required property var modelData
                    spacing: 24
                    width: parent.width
                    Text {
                        width: 100
                        text: Theme.formatTime(modelData.start)
                        color: Theme.textDim
                        font.pixelSize: Theme.fontNormal
                        font.features: {
                            "tnum": 1
                        }
                    }
                    Rectangle {
                        width: 6
                        height: 28
                        radius: 3
                        anchors.verticalCenter: parent.verticalCenter
                        color: Theme.genreColor(modelData.genre)
                    }
                    Text {
                        width: parent.width - 160
                        text: modelData.title
                        color: Theme.text
                        font.pixelSize: Theme.fontNormal
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Row {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 40
            spacing: 36
            Repeater {
                model: [
                    {
                        key: "OK",
                        label: qsTr("Watch")
                    },
                    {
                        key: "▶",
                        label: qsTr("Guide")
                    },
                    {
                        key: "0-9",
                        label: qsTr("Channel number")
                    }
                ]
                delegate: Row {
                    required property var modelData
                    spacing: 12
                    Rectangle {
                        width: Math.max(44, keyText.implicitWidth + 20)
                        height: 36
                        radius: 8
                        color: "transparent"
                        border.color: Theme.textFaint
                        border.width: 2
                        Text {
                            id: keyText
                            anchors.centerIn: parent
                            text: modelData.key
                            color: Theme.textDim
                            font.pixelSize: Theme.fontTiny
                            font.weight: Font.DemiBold
                        }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.label
                        color: Theme.textFaint
                        font.pixelSize: Theme.fontTiny
                    }
                }
            }
        }
    }

    // Empty / first-run states
    Column {
        anchors.centerIn: parent
        visible: root.empty
        spacing: 28
        width: 1100

        QQC2.BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 96
            height: 96
            visible: Tvh.status === Tvh.Connecting || Tvh.busy
            running: visible
        }
        Kirigami.Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 160
            height: 160
            visible: !(Tvh.status === Tvh.Connecting || Tvh.busy)
            source: Tvh.status === Tvh.Offline ? "network-disconnect" : "video-television"
            color: Theme.textFaint
            isMask: true
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: Tvh.status === Tvh.Unconfigured ? qsTr("Welcome!") : Tvh.status === Tvh.Offline ? qsTr("Can't reach the server") : Tvh.status === Tvh.Online && !Tvh.busy ? qsTr("No channels found") : qsTr("Loading channels…")
            color: Theme.text
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: Tvh.status === Tvh.Unconfigured ? qsTr("Open Settings (press ▲ and go to Settings, or the MENU key) to connect to your TVHeadend server.") : Tvh.status === Tvh.Offline ? Tvh.statusText : Tvh.status === Tvh.Online && !Tvh.busy ? qsTr("The server has no enabled channels for this user.") : ""
            color: Theme.textDim
            font.pixelSize: Theme.fontNormal
        }
    }
}
