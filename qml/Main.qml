import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import PlasmaTv

QQC2.ApplicationWindow {
    id: window
    property bool startWindowed: false
    // "home": navigation + pages, "watch": fullscreen video with OSD
    property string mode: "home"
    property string page: Settings.configured ? "channels" : "settings"
    property string numberEntry: ""
    property bool cursorVisible: true
    // Channel stopped because the window lost focus; resumed when it returns.
    property string backgroundResumeUuid

    width: 1600
    height: 900
    visible: true
    visibility: startWindowed ? Window.Windowed : Window.FullScreen
    title: qsTr("Plasma TV")
    color: "black"

    onActiveChanged: {
        if (!active) {
            if (Settings.stopInBackground && player.active)
                backgroundTimer.restart();
            return;
        }
        backgroundTimer.stop();
        if (backgroundResumeUuid.length > 0) {
            player.play(backgroundResumeUuid);
            backgroundResumeUuid = "";
        }
    }

    function pageItem(name) {
        return name === "guide" ? guidePage : name === "settings" ? settingsPage : channelsPage;
    }

    function goPage(name, focusContent) {
        mode = "home";
        page = name;
        if (focusContent)
            pageItem(name).forceActiveFocus();
        else
            nav.forceActiveFocus();
    }

    function watch(uuid) {
        if (!uuid)
            return;
        if (uuid !== player.channelUuid || player.error.length > 0)
            player.play(uuid);
        Settings.lastChannel = uuid;
        mode = "watch";
        overlay.forceActiveFocus();
        overlay.showInfo();
    }

    function leaveWatch(target) {
        const index = Tvh.channels.indexOfUuid(player.channelUuid);
        if (target === "guide") {
            goPage("guide", true);
            guidePage.showChannel(player.channelUuid);
        } else {
            goPage("channels", true);
            if (index >= 0)
                channelsPage.selectIndex(index);
        }
    }

    function commitNumber() {
        const number = parseInt(numberEntry);
        numberEntry = "";
        const index = Tvh.channels.indexOfNumber(number);
        if (index < 0) {
            toast.show(qsTr("Channel %1 not found").arg(number));
            return;
        }
        const uuid = Tvh.channels.get(index).uuid;
        if (mode === "watch")
            overlay.play(uuid);
        else if (page === "guide") {
            guidePage.selectIndex(index);
            guidePage.forceActiveFocus();
        } else {
            goPage("channels", true);
            channelsPage.selectIndex(index);
        }
    }

    Binding {
        target: Theme
        property: "source"
        value: stage
    }

    Timer {
        id: backgroundTimer
        // Short focus changes (notifications, dialogs) shouldn't interrupt the stream.
        interval: 1500
        onTriggered: {
            if (window.active || !player.active)
                return;
            window.backgroundResumeUuid = player.channelUuid;
            player.stop();
        }
    }

    ScreenSaverInhibitor {
        active: player.active && window.mode === "watch"
    }

    PlayerView {
        id: player
        anchors.fill: parent
    }

    // The whole UI is laid out on a 1920x1080 canvas and scaled to the screen.
    Item {
        id: stage
        width: 1920
        height: 1080
        anchors.centerIn: parent
        scale: Math.min(window.width / width, window.height / height)

        FocusScope {
            id: keyRoot
            anchors.fill: parent
            focus: true

            // Global remote keys; reached when the focused item doesn't handle a key.
            Keys.onPressed: event => {
                const k = event.key;
                if (k >= Qt.Key_0 && k <= Qt.Key_9 && Tvh.channels.count > 0) {
                    window.numberEntry = (window.numberEntry + event.text).slice(-4);
                    numberTimer.restart();
                } else if (k === Qt.Key_Menu || k === Qt.Key_Settings || k === Qt.Key_F2) {
                    window.goPage("settings", true);
                } else if (k === Qt.Key_Guide || k === Qt.Key_G || k === Qt.Key_F3) {
                    window.goPage("guide", true);
                    if (player.active)
                        guidePage.showChannel(player.channelUuid);
                } else if (k === Qt.Key_F11) {
                    window.visibility = window.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen;
                } else if (k === Qt.Key_MediaStop) {
                    player.stop();
                } else if (Theme.isBack(k) && window.mode === "home") {
                    if (!nav.activeFocus)
                        nav.forceActiveFocus();
                    else if (player.active)
                        window.watch(player.channelUuid);
                } else {
                    return;
                }
                event.accepted = true;
            }

            // --- Home -------------------------------------------------------
            FocusScope {
                id: home
                anchors.fill: parent
                visible: window.mode === "home"
                focus: true

                Rectangle {
                    anchors.fill: parent
                    // A playing channel shines through behind the menus.
                    opacity: player.active ? 0.9 : 1
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: Theme.backgroundTop
                        }
                        GradientStop {
                            position: 0.35
                            color: Theme.background
                        }
                        GradientStop {
                            position: 1
                            color: Theme.background
                        }
                    }
                }

                NavBar {
                    id: nav
                    width: parent.width
                    currentPage: window.page
                    onPageSelected: name => window.page = name
                    onEnterPage: window.pageItem(window.page).forceActiveFocus()
                }

                ChannelsPage {
                    id: channelsPage
                    y: nav.height
                    width: parent.width
                    height: parent.height - nav.height
                    visible: window.page === "channels"
                    onExitUp: nav.forceActiveFocus()
                    onWatch: uuid => window.watch(uuid)
                    onOpenGuide: uuid => {
                        window.goPage("guide", true);
                        guidePage.showChannel(uuid);
                    }
                }

                GuidePage {
                    id: guidePage
                    y: nav.height
                    width: parent.width
                    height: parent.height - nav.height
                    visible: window.page === "guide"
                    onExitUp: nav.forceActiveFocus()
                    onWatch: uuid => window.watch(uuid)
                    onOpenEvent: ev => eventDialog.open(ev)
                }

                SettingsPage {
                    id: settingsPage
                    y: nav.height
                    width: parent.width
                    height: parent.height - nav.height
                    visible: window.page === "settings"
                    onExitUp: nav.forceActiveFocus()
                    onEditText: (title, value, mode, callback) => textEntry.open(title, value, mode, callback)
                }
            }

            // --- Watching -----------------------------------------------------
            WatchOverlay {
                id: overlay
                anchors.fill: parent
                player: player
                visible: window.mode === "watch"
                onLeave: target => window.leaveWatch(target)
            }

            EventDialog {
                id: eventDialog
                anchors.fill: parent
                onWatch: uuid => window.watch(uuid)
            }

            TextEntryDialog {
                id: textEntry
                anchors.fill: parent
            }
        }

        // Channel number being typed on the remote
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 64
            visible: window.numberEntry.length > 0
            width: Math.max(200, numberText.implicitWidth + 80)
            height: 150
            radius: Theme.radius
            color: Theme.background
            border.color: Theme.accent
            border.width: 3
            z: 200
            Text {
                id: numberText
                anchors.centerIn: parent
                text: window.numberEntry
                color: Theme.text
                font.pixelSize: Theme.fontHuge
                font.weight: Font.Light
            }
        }

        Toast {
            id: toast
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 72
            z: 300
        }
    }

    Timer {
        id: numberTimer
        interval: 1600
        onTriggered: window.commitNumber()
    }

    Connections {
        target: Tvh
        function onNotification(message) {
            toast.show(message);
        }
    }

    Connections {
        target: Settings
        function onSaved(serverChanged) {
            toast.show(qsTr("Settings saved"));
        }
    }

    // Hide the mouse pointer when it isn't used – this is a TV.
    MouseArea {
        anchors.fill: parent
        z: 1000
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        cursorShape: window.cursorVisible ? Qt.ArrowCursor : Qt.BlankCursor
        onPositionChanged: {
            window.cursorVisible = true;
            cursorTimer.restart();
        }
        Timer {
            id: cursorTimer
            interval: 3000
            running: true
            onTriggered: window.cursorVisible = false
        }
    }

    Component.onCompleted: {
        if (Settings.configured) {
            window.goPage("channels", true);
            const index = Tvh.channels.indexOfUuid(Settings.lastChannel);
            if (index >= 0)
                channelsPage.selectIndex(index);
        } else {
            window.goPage("settings", true);
        }
    }
}
