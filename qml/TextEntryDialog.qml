// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

// Text input. Typing is done with a physical keyboard or Plasma's virtual
// keyboard (plasma-keyboard), which KWin shows for the focused field.
FocusScope {
    id: root
    property string title
    property string mode: "text"   // "text" | "password" | "number"
    property var callback: null
    property Item returnFocus: null
    readonly property QQC2.TextField field: mode === "password" ? passwordField : textField

    visible: false
    z: 100

    function open(title, value, mode, callback) {
        root.title = title;
        root.mode = mode;
        root.callback = callback;
        root.returnFocus = Window.activeFocusItem;
        textField.text = mode === "password" ? "" : value;
        passwordField.text = mode === "password" ? value : "";
        root.visible = true;
        root.field.forceActiveFocus();
        root.field.selectAll();
        Qt.inputMethod.show();
    }

    function close(accept) {
        if (!visible)
            return;
        Qt.inputMethod.hide();
        visible = false;
        if (accept && callback)
            callback(field.text);
        callback = null;
        if (returnFocus)
            returnFocus.forceActiveFocus();
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back || event.key === Qt.Key_Exit) {
            close(false);
            event.accepted = true;
        } else if (event.key === Qt.Key_Up) {
            field.forceActiveFocus();
            event.accepted = true;
        } else if (event.key >= Qt.Key_0 && event.key <= Qt.Key_9) {
            event.accepted = true;   // keep digits away from the global channel entry
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.6)
        MouseArea {
            anchors.fill: parent
            onClicked: root.close(false)
        }
    }

    Rectangle {
        id: card
        // Keep clear of the lower half, where the virtual keyboard slides in.
        anchors.horizontalCenter: parent.horizontalCenter
        y: 140
        width: 1200
        height: content.implicitHeight + 96
        radius: Theme.radius + 12
        color: Theme.background
        border.color: Theme.outline
        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: content
            anchors.fill: parent
            anchors.margins: 48
            spacing: 32

            Text {
                text: root.title
                color: Theme.text
                font.pixelSize: Theme.fontLarge
            }

            QQC2.TextField {
                id: textField
                visible: root.mode !== "password"
                width: parent.width
                font.pixelSize: Theme.fontLarge
                inputMethodHints: root.mode === "number" ? Qt.ImhDigitsOnly : Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhUrlCharactersOnly
                validator: root.mode === "number" ? portValidator : null
                onAccepted: root.close(true)
                Keys.onDownPressed: okButton.forceActiveFocus()
                onActiveFocusChanged: {
                    if (activeFocus) {
                        Qt.inputMethod.show();
                    }
                }
            }

            Kirigami.PasswordField {
                id: passwordField
                visible: root.mode === "password"
                width: parent.width
                font.pixelSize: Theme.fontLarge
                inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                onAccepted: root.close(true)
                Keys.onDownPressed: okButton.forceActiveFocus()
                onActiveFocusChanged: {
                    if (activeFocus) {
                        Qt.inputMethod.show();
                    }
                }
            }

            Row {
                spacing: 24
                TvButton {
                    id: okButton
                    primary: true
                    text: qsTr("OK")
                    iconName: "dialog-ok"
                    onClicked: root.close(true)
                    KeyNavigation.right: cancelButton
                }
                TvButton {
                    id: cancelButton
                    text: qsTr("Cancel")
                    iconName: "dialog-cancel"
                    onClicked: root.close(false)
                    KeyNavigation.left: okButton
                }
            }

            Text {
                width: parent.width
                text: qsTr("Type with Plasma's on-screen keyboard or a keyboard · OK/Enter confirms · BACK cancels")
                color: Theme.textFaint
                font.pixelSize: Theme.fontTiny
                wrapMode: Text.Wrap
            }
        }
    }

    IntValidator {
        id: portValidator
        bottom: 1
        top: 65535
    }
}
