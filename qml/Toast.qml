import QtQuick

Rectangle {
    id: root
    function show(message) {
        label.text = message;
        opacity = 1;
        hideTimer.restart();
    }

    width: Math.min(1400, label.implicitWidth + 80)
    height: label.implicitHeight + 40
    radius: height / 2
    color: Theme.surfaceRaised
    border.color: Theme.outline
    opacity: 0
    visible: opacity > 0
    Behavior on opacity {
        NumberAnimation {
            duration: Theme.animNormal
        }
    }

    Text {
        id: label
        anchors.centerIn: parent
        width: Math.min(implicitWidth, 1320)
        color: Theme.text
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }

    Timer {
        id: hideTimer
        interval: 4000
        onTriggered: root.opacity = 0
    }
}
