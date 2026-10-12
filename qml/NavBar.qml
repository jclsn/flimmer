import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import PlasmaTv

// Top navigation. Moving left/right switches pages immediately; down/OK enters the page.
FocusScope {
    id: root
    property string currentPage: "channels"
    signal pageSelected(string page)
    signal enterPage

    readonly property var pages: [
        {
            id: "channels",
            label: qsTr("Live TV"),
            icon: "video-television"
        },
        {
            id: "guide",
            label: qsTr("TV Guide"),
            icon: "view-calendar-timeline"
        },
        {
            id: "settings",
            label: qsTr("Settings"),
            icon: "configure"
        }
    ]
    readonly property int currentIndex: Math.max(0, pages.findIndex(p => p.id === currentPage))

    implicitHeight: 128

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Left && currentIndex > 0) {
            pageSelected(pages[currentIndex - 1].id);
            event.accepted = true;
        } else if (event.key === Qt.Key_Right && currentIndex < pages.length - 1) {
            pageSelected(pages[currentIndex + 1].id);
            event.accepted = true;
        } else if (event.key === Qt.Key_Down || Theme.isAccept(event.key)) {
            enterPage();
            event.accepted = true;
        }
    }

    Row {
        id: brand
        anchors.left: parent.left
        anchors.leftMargin: Theme.margin
        anchors.verticalCenter: parent.verticalCenter
        spacing: 16

        Kirigami.Icon {
            anchors.verticalCenter: parent.verticalCenter
            width: 56
            height: 56
            source: "video-television"
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "Plasma TV"
            color: Theme.text
            font.pixelSize: Theme.fontLarge
            font.weight: Font.DemiBold
        }
    }

    Row {
        id: tabs
        anchors.centerIn: parent
        spacing: 12

        Repeater {
            model: root.pages
            delegate: Item {
                required property var modelData
                required property int index
                readonly property bool selected: index === root.currentIndex
                readonly property bool focused: selected && root.activeFocus
                width: tabContent.implicitWidth + 64
                height: 68

                Rectangle {
                    anchors.fill: parent
                    radius: height / 2
                    color: parent.focused ? Theme.accent : (parent.selected ? Theme.surfaceRaised : "transparent")
                    border.width: parent.focused ? 3 : 0
                    border.color: Theme.focusRing
                    scale: parent.focused ? 1.06 : 1
                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.animFast
                        }
                    }
                    Behavior on scale {
                        NumberAnimation {
                            duration: Theme.animFast
                        }
                    }
                }
                Row {
                    id: tabContent
                    anchors.centerIn: parent
                    spacing: 14
                    Kirigami.Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 30
                        height: 30
                        source: parent.parent.modelData.icon
                        isMask: true
                        color: tabLabel.color
                    }
                    Text {
                        id: tabLabel
                        anchors.verticalCenter: parent.verticalCenter
                        text: parent.parent.modelData.label
                        color: parent.parent.focused ? Theme.onAccent : (parent.parent.selected ? Theme.text : Theme.textDim)
                        font.pixelSize: Theme.fontNormal
                        font.weight: parent.parent.selected ? Font.DemiBold : Font.Normal
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.pageSelected(parent.modelData.id);
                        root.enterPage();
                    }
                }
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: Theme.margin
        anchors.verticalCenter: parent.verticalCenter
        spacing: 28

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            visible: Tvh.status !== Tvh.Online || Tvh.busy
            QQC2.BusyIndicator {
                width: 40
                height: 40
                running: visible
                anchors.verticalCenter: parent.verticalCenter
                visible: Tvh.busy || Tvh.status === Tvh.Connecting
            }
            Rectangle {
                width: 14
                height: 14
                radius: 7
                anchors.verticalCenter: parent.verticalCenter
                visible: !Tvh.busy && Tvh.status !== Tvh.Connecting
                color: Tvh.status === Tvh.Offline ? Theme.danger : Theme.warning
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: Tvh.busy && Tvh.status === Tvh.Online ? qsTr("Updating guide…") : Tvh.statusText
                color: Theme.textDim
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
                width: Math.min(implicitWidth, 420)
            }
        }

        Text {
            id: clock
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.text
            font.pixelSize: Theme.fontLarge
            font.weight: Font.Light
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
