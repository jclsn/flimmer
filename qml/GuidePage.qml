// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import PlasmaTv

// Electronic programme guide: channels vertically, time horizontally.
FocusScope {
    id: root
    signal exitUp
    signal watch(string uuid)
    signal openEvent(var event)
    signal openSettings

    readonly property real slotMs: 30 * 60 * 1000
    readonly property real spanMs: 3 * 3600 * 1000
    readonly property int channelColumnWidth: 220
    readonly property int rowHeight: 64
    readonly property real gridWidth: width - 2 * Theme.margin - channelColumnWidth
    readonly property real pxPerMs: gridWidth / spanMs
    readonly property real minTime: floorSlot(Tvh.now)
    readonly property real maxTime: Math.max(Tvh.epgEnd, Tvh.now + spanMs)

    property real windowStart: floorSlot(Date.now())
    readonly property real windowEnd: windowStart + spanMs
    property real focusTime: Date.now()
    property var selected: ({})
    readonly property bool empty: Tvh.channels.count === 0
    property alias currentIndex: rows.currentIndex

    function floorSlot(ms) {
        return Math.floor(ms / slotMs) * slotMs;
    }

    function currentUuid() {
        const ch = Tvh.channels.get(rows.currentIndex);
        return ch.uuid ?? "";
    }

    function reselect() {
        if (rows.currentIndex < 0 && rows.count > 0)
            rows.currentIndex = 0;
        if (windowStart < minTime)
            windowStart = minTime;
        if (focusTime < Date.now())
            focusTime = Date.now();
        selected = Tvh.eventAt(currentUuid(), focusTime);
    }

    function ensureVisible() {
        if (focusTime < windowStart)
            windowStart = Math.max(minTime, floorSlot(focusTime));
        else if (focusTime >= windowEnd - slotMs)
            windowStart = Math.max(minTime, floorSlot(focusTime) - slotMs);
    }

    function moveTo(time) {
        focusTime = time;
        ensureVisible();
        selected = Tvh.eventAt(currentUuid(), focusTime);
    }

    function showChannel(uuid) {
        const index = Tvh.channels.indexOfUuid(uuid);
        selectIndex(index >= 0 ? index : 0);
        windowStart = minTime;
        moveTo(Date.now());
    }

    function selectIndex(index) {
        if (index < 0 || index >= rows.count)
            return;
        rows.currentIndex = index;
        rows.positionViewAtIndex(index, ListView.Contain);
        selected = Tvh.eventAt(currentUuid(), focusTime);
    }

    function stepLeft() {
        const t = selected.id !== undefined ? selected.start - 1 : focusTime - slotMs;
        if (t < minTime)
            return;
        const ev = Tvh.eventAt(currentUuid(), t);
        moveTo(ev.id !== undefined ? Math.max(ev.start, minTime) : Math.max(floorSlot(t), minTime));
    }

    function stepRight() {
        const t = selected.id !== undefined ? selected.stop : floorSlot(focusTime) + slotMs;
        if (t >= maxTime)
            return;
        const ev = Tvh.eventAt(currentUuid(), t);
        if (ev.id !== undefined) {
            moveTo(t);
            return;
        }
        const next = Tvh.nextEvent(currentUuid(), t);
        moveTo(next.id !== undefined && next.start < t + slotMs ? next.start : t);
    }

    function jump(deltaMs) {
        const t = Math.min(maxTime - slotMs, Math.max(Date.now(), focusTime + deltaMs));
        windowStart = Math.max(minTime, floorSlot(t));
        moveTo(Math.max(t, Date.now()));
    }

    onVisibleChanged: {
        if (visible) {
            reselect();
        }
    }

    Connections {
        target: Tvh
        function onEpgChanged() {
            root.reselect();
        }
        function onTick() {
            if (root.windowStart < root.minTime)
                root.reselect();
        }
    }

    // --- Selected programme details -------------------------------------
    Item {
        id: header
        visible: !root.empty
        x: Theme.margin
        width: parent.width - 2 * Theme.margin
        height: 147

        readonly property var channel: {
            Tvh.epgRevision;
            return Tvh.channels.get(rows.currentIndex);
        }
        readonly property bool hasEvent: root.selected.id !== undefined

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 5
            spacing: 7

            Row {
                spacing: 12
                Text {
                    text: header.channel.name ?? ""
                    color: Theme.accent
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 1
                }
                Text {
                    visible: header.hasEvent
                    text: header.hasEvent ? Theme.formatDay(root.selected.start) + "  ·  " + Theme.formatRange(root.selected.start, root.selected.stop) + "  ·  " + Theme.formatDuration(root.selected.stop - root.selected.start) : ""
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSmall
                }
                Rectangle {
                    visible: header.hasEvent && root.selected.genreName.length > 0
                    width: genreText.implicitWidth + 16
                    height: genreText.implicitHeight + 4
                    radius: height / 2
                    color: Theme.genreColor(root.selected.genre ?? 0)
                    Text {
                        id: genreText
                        anchors.centerIn: parent
                        text: root.selected.genreName ?? ""
                        color: "white"
                        font.pixelSize: Theme.fontTiny
                        font.weight: Font.DemiBold
                    }
                }
                Text {
                    visible: header.hasEvent && (root.selected.dvrState ?? "").length > 0
                    text: root.selected.dvrState === "recording" ? qsTr("● Recording") : qsTr("● Scheduled")
                    color: Theme.recording
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
                }
            }
            Text {
                width: parent.width
                text: header.hasEvent ? root.selected.title : qsTr("No programme information")
                color: Theme.text
                font.pixelSize: Theme.fontTitle
                font.weight: Font.Bold
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                visible: header.hasEvent
                text: {
                    if (!header.hasEvent)
                        return "";
                    const parts = [];
                    if (root.selected.subtitle.length > 0)
                        parts.push(root.selected.subtitle);
                    const desc = root.selected.summary.length > 0 ? root.selected.summary : root.selected.description;
                    if (desc.length > 0 && desc !== root.selected.subtitle)
                        parts.push(desc);
                    return parts.join(" — ");
                }
                color: Theme.textDim
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                lineHeight: 1.15
            }
        }
    }

    // --- Timeline header ------------------------------------------------
    Item {
        id: timeline
        visible: !root.empty
        anchors.top: header.bottom
        x: Theme.margin
        width: parent.width - 2 * Theme.margin
        height: 35

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: Theme.formatDay(root.windowStart)
            color: Theme.text
            font.pixelSize: Theme.fontNormal
            font.weight: Font.DemiBold
        }

        Repeater {
            model: root.spanMs / root.slotMs
            delegate: Item {
                required property int index
                x: root.channelColumnWidth + index * root.slotMs * root.pxPerMs
                height: parent.height
                Rectangle {
                    width: 1
                    height: 9
                    anchors.bottom: parent.bottom
                    color: Theme.outline
                }
                Text {
                    x: 7
                    anchors.verticalCenter: parent.verticalCenter
                    text: Theme.formatTime(root.windowStart + parent.index * root.slotMs)
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSmall
                }
            }
        }
    }

    // --- Grid -----------------------------------------------------------
    ListView {
        id: rows
        visible: !root.empty
        anchors.top: timeline.bottom
        anchors.topMargin: 7
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        x: Theme.margin
        width: parent.width - 2 * Theme.margin
        model: Tvh.channels
        focus: !root.empty
        clip: true
        spacing: 5
        highlightMoveDuration: 140
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: rowHeight + spacing
        preferredHighlightEnd: height - 2 * (rowHeight + spacing)
        cacheBuffer: 400

        onCurrentIndexChanged: root.selected = Tvh.eventAt(root.currentUuid(), root.focusTime)

        Keys.onPressed: event => {
            switch (event.key) {
            case Qt.Key_Up:
                if (currentIndex <= 0)
                    root.exitUp();
                else
                    decrementCurrentIndex();
                break;
            case Qt.Key_Down:
                incrementCurrentIndex();
                break;
            case Qt.Key_Left:
                root.stepLeft();
                break;
            case Qt.Key_Right:
                root.stepRight();
                break;
            case Qt.Key_PageUp:
            case Qt.Key_ChannelUp:
                currentIndex = Math.max(0, currentIndex - 6);
                break;
            case Qt.Key_PageDown:
            case Qt.Key_ChannelDown:
                currentIndex = Math.min(count - 1, currentIndex + 6);
                break;
            case Qt.Key_MediaNext:
            case Qt.Key_Forward:
            case Qt.Key_F:
                root.jump(24 * 3600 * 1000);
                break;
            case Qt.Key_MediaPrevious:
            case Qt.Key_Rewind:
            case Qt.Key_R:
                root.jump(-24 * 3600 * 1000);
                break;
            default:
                if (Theme.isAccept(event.key) && !event.isAutoRepeat) {
                    if (root.selected.id !== undefined)
                        root.openEvent(root.selected);
                    else
                        root.watch(root.currentUuid());
                    break;
                }
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
            readonly property bool isCurrent: ListView.isCurrentItem
            readonly property bool focused: isCurrent && rows.activeFocus
            readonly property var events: {
                Tvh.epgRevision;
                return Tvh.eventsForChannel(uuid, root.windowStart, root.windowEnd);
            }

            width: ListView.view.width
            height: root.rowHeight

            Rectangle {
                id: channelCell
                width: root.channelColumnWidth - 8
                height: parent.height
                radius: Theme.radiusSmall
                color: row.isCurrent ? Theme.surfaceHover : Theme.surface
                border.width: row.focused ? 1 : 0
                border.color: Theme.accent

                Text {
                    id: num
                    anchors.left: parent.left
                    anchors.leftMargin: 9
                    anchors.verticalCenter: parent.verticalCenter
                    width: 31
                    horizontalAlignment: Text.AlignRight
                    text: row.number > 0 ? row.number : ""
                    color: Theme.textFaint
                    font.pixelSize: Theme.fontSmall
                }
                ChannelLogo {
                    id: logo
                    anchors.left: num.right
                    anchors.leftMargin: 9
                    anchors.verticalCenter: parent.verticalCenter
                    width: 64
                    height: 40
                    source: row.iconUrl
                    name: row.name
                }
                Text {
                    anchors.left: logo.right
                    anchors.leftMargin: 9
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.name
                    color: row.isCurrent ? Theme.text : Theme.textDim
                    font.pixelSize: Theme.fontSmall
                    font.weight: row.isCurrent ? Font.DemiBold : Font.Normal
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }

            Item {
                x: root.channelColumnWidth
                width: root.gridWidth
                height: parent.height
                clip: true

                Rectangle {
                    visible: row.events.length === 0
                    anchors.fill: parent
                    anchors.rightMargin: 4
                    radius: Theme.radiusSmall
                    color: row.focused ? Theme.accentDeep : Theme.surface
                    border.width: row.focused ? 2 : 0
                    border.color: Theme.focusRing
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("No information")
                        color: Theme.textFaint
                        font.pixelSize: Theme.fontSmall
                    }
                }

                Repeater {
                    model: row.events
                    delegate: Rectangle {
                        id: cell
                        required property var modelData
                        readonly property bool selected: row.isCurrent && modelData.id === root.selected.id
                        readonly property bool active: selected && rows.activeFocus
                        readonly property bool airing: modelData.start <= Tvh.now && modelData.stop > Tvh.now
                        readonly property real clipStart: Math.max(modelData.start, root.windowStart)
                        readonly property real clipStop: Math.min(modelData.stop, root.windowEnd)

                        x: (clipStart - root.windowStart) * root.pxPerMs
                        width: Math.max(3, (clipStop - clipStart) * root.pxPerMs - 4)
                        height: row.height
                        radius: Theme.radiusSmall
                        color: active ? Theme.accent : selected ? Theme.accentDeep : airing ? Theme.surfaceRaised : Theme.surface
                        border.width: active ? 2 : 0
                        border.color: Theme.focusRing
                        z: selected ? 1 : 0

                        Rectangle {
                            width: 3
                            radius: 1
                            anchors.left: parent.left
                            anchors.leftMargin: 5
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: 11
                            color: Theme.genreColor(cell.modelData.genre)
                            visible: parent.width > 27 && !cell.active
                        }

                        Column {
                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            anchors.right: parent.right
                            anchors.rightMargin: 9
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 3
                            visible: parent.width > 40
                            Row {
                                width: parent.width
                                spacing: 5
                                Rectangle {
                                    visible: cell.modelData.dvrState.length > 0
                                    width: 8
                                    height: 8
                                    radius: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: Theme.recording
                                }
                                Text {
                                    width: parent.width - (cell.modelData.dvrState.length > 0 ? 13 : 0)
                                    text: cell.modelData.title
                                    color: cell.active ? Theme.onAccent : Theme.text
                                    font.pixelSize: Theme.fontSmall
                                    font.weight: cell.selected || cell.airing ? Font.DemiBold : Font.Normal
                                    elide: Text.ElideRight
                                }
                            }
                            Text {
                                width: parent.width
                                text: Theme.formatRange(cell.modelData.start, cell.modelData.stop)
                                color: cell.active ? Theme.onAccent : Theme.textFaint
                                font.pixelSize: Theme.fontTiny
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                rows.forceActiveFocus();
                                if (cell.selected) {
                                    root.openEvent(cell.modelData);
                                    return;
                                }
                                rows.currentIndex = row.index;
                                root.focusTime = Math.max(cell.modelData.start, root.minTime);
                                root.selected = cell.modelData;
                            }
                        }
                    }
                }
            }
        }
    }

    // "Now" marker across the grid
    Rectangle {
        visible: !root.empty && Tvh.now >= root.windowStart && Tvh.now < root.windowEnd
        x: Theme.margin + root.channelColumnWidth + (Tvh.now - root.windowStart) * root.pxPerMs - 1
        y: timeline.y + timeline.height - 11
        width: 2
        height: rows.y + rows.height - y
        color: Theme.danger
        opacity: 0.85
        z: 5
        Rectangle {
            width: 9
            height: 9
            radius: 5
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.danger
        }
    }

    BigscreenUI.PlaceholderMessage {
        anchors.centerIn: parent
        width: 733
        visible: root.empty
        focus: root.empty
        iconName: "view-calendar-timeline"
        text: Tvh.status === Tvh.Unconfigured ? qsTr("Configure your server in Settings to see the TV guide") : qsTr("No guide data yet")
        helpfulAction: Kirigami.Action {
            text: qsTr("Open Settings")
            icon.name: "configure"
            visible: Tvh.status === Tvh.Unconfigured || Tvh.status === Tvh.Offline
            onTriggered: root.openSettings()
        }
    }

    Keys.onUpPressed: root.exitUp()
}
