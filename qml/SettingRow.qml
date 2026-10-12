import QtQuick

// One focusable settings line. kind: "text" (OK edits), "toggle" (OK flips),
// "stepper" (left/right change the value).
FocusScope {
    id: root
    property string label
    property string value
    property string placeholder
    property string kind: "text"
    property bool checked: false
    signal activated
    signal decrement
    signal increment

    implicitHeight: 72
    activeFocusOnTab: true

    Keys.onPressed: event => {
        if (Theme.isAccept(event.key) && !event.isAutoRepeat) {
            root.activated();
            event.accepted = true;
        } else if (kind === "stepper" && event.key === Qt.Key_Left) {
            root.decrement();
            event.accepted = true;
        } else if (kind === "stepper" && event.key === Qt.Key_Right) {
            root.increment();
            event.accepted = true;
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: root.activeFocus ? Theme.surfaceHover : "transparent"
        border.width: root.activeFocus ? 3 : 0
        border.color: Theme.focusRing
        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 32
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: Theme.text
        font.pixelSize: Theme.fontNormal
    }

    // text / stepper value
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 32
        anchors.verticalCenter: parent.verticalCenter
        spacing: 20
        visible: root.kind !== "toggle"

        Text {
            visible: root.kind === "stepper"
            text: "‹"
            color: root.activeFocus ? Theme.text : Theme.textFaint
            font.pixelSize: Theme.fontLarge
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, 560)
            horizontalAlignment: Text.AlignRight
            text: root.value.length > 0 ? root.value : root.placeholder
            color: root.value.length > 0 ? Theme.textDim : Theme.textFaint
            font.pixelSize: Theme.fontNormal
            font.italic: root.value.length === 0
            elide: Text.ElideMiddle
        }
        Text {
            visible: root.kind === "stepper"
            text: "›"
            color: root.activeFocus ? Theme.text : Theme.textFaint
            font.pixelSize: Theme.fontLarge
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: root.kind === "text"
            text: "✎"
            color: root.activeFocus ? Theme.accent : Theme.textFaint
            font.pixelSize: Theme.fontSmall
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // toggle switch
    Rectangle {
        visible: root.kind === "toggle"
        anchors.right: parent.right
        anchors.rightMargin: 32
        anchors.verticalCenter: parent.verticalCenter
        width: 92
        height: 48
        radius: 24
        color: root.checked ? Theme.accent : Theme.outline
        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
            }
        }
        Rectangle {
            width: 38
            height: 38
            radius: 19
            y: 5
            x: root.checked ? parent.width - width - 5 : 5
            color: "white"
            Behavior on x {
                NumberAnimation {
                    duration: Theme.animFast
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.forceActiveFocus();
            root.activated();
        }
    }
}
