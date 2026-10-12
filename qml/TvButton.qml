import QtQuick
import org.kde.kirigami as Kirigami

// Focusable pill button for remote control navigation.
FocusScope {
    id: root
    property string text
    property string iconName    // optional Breeze icon name
    property bool primary: false
    property bool destructive: false
    // Not Item.enabled: a disabled item would drop the remote focus.
    property bool available: true
    signal clicked

    implicitWidth: Math.max(220, label.implicitWidth + (glyph.visible ? glyph.implicitWidth + 16 : 0) + 72)
    implicitHeight: 72
    activeFocusOnTab: true
    opacity: available ? 1 : 0.4

    Keys.onPressed: event => {
        if (Theme.isAccept(event.key) && !event.isAutoRepeat) {
            if (root.available)
                root.clicked();
            event.accepted = true;
        }
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: height / 2
        color: root.activeFocus ? (root.destructive ? Theme.danger : Theme.accent) : (root.primary ? Theme.accentDeep : Theme.surfaceRaised)
        border.width: root.activeFocus ? 3 : 0
        border.color: Theme.focusRing
        scale: root.activeFocus ? 1.06 : 1
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
    }

    Row {
        anchors.centerIn: parent
        spacing: 16
        scale: bg.scale
        Kirigami.Icon {
            id: glyph
            visible: root.iconName.length > 0
            source: root.iconName
            implicitWidth: 32
            implicitHeight: 32
            isMask: true
            color: label.color
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: label
            text: root.text
            color: root.activeFocus || root.primary ? Theme.onAccent : Theme.text
            font.pixelSize: Theme.fontNormal
            font.weight: root.activeFocus ? Font.DemiBold : Font.Normal
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.forceActiveFocus();
            if (root.available)
                root.clicked();
        }
    }
}
