import QtQuick
import QtMultimedia
import PlasmaTv

// Video layer. Lives outside the scaled UI so it always renders at native resolution.
Item {
    id: root
    property string channelUuid
    property string error
    property bool awaitingUrl: false
    property int retries: 0
    readonly property bool active: channelUuid.length > 0
    readonly property alias mediaPlayer: player

    function play(uuid) {
        if (uuid !== channelUuid)
            retries = 0;
        player.stop();
        player.source = "";
        channelUuid = uuid;
        error = "";
        awaitingUrl = true;
        Tvh.requestStream(uuid);
    }

    function retry() {
        if (channelUuid.length > 0)
            play(channelUuid);
    }

    function stop() {
        player.stop();
        player.source = "";
        channelUuid = "";
        error = "";
        awaitingUrl = false;
    }

    // Cycle helpers return a human readable description for the OSD.
    function trackName(meta, index) {
        const lang = meta ? meta.stringValue(MediaMetaData.Language) : "";
        const title = meta ? meta.stringValue(MediaMetaData.Title) : "";
        const parts = [lang, title].filter(s => s && s.length > 0);
        return parts.length > 0 ? parts.join(" – ") : qsTr("Track %1").arg(index + 1);
    }

    function cycleAudio() {
        const n = player.audioTracks.length;
        if (n < 2)
            return qsTr("Only one audio track");
        player.activeAudioTrack = (player.activeAudioTrack + 1) % n;
        return qsTr("Audio: %1").arg(trackName(player.audioTracks[player.activeAudioTrack], player.activeAudioTrack));
    }

    function cycleSubtitles() {
        const n = player.subtitleTracks.length;
        if (n === 0)
            return qsTr("No subtitles available");
        const next = player.activeSubtitleTrack + 1;
        player.activeSubtitleTrack = next >= n ? -1 : next;
        if (player.activeSubtitleTrack < 0)
            return qsTr("Subtitles off");
        return qsTr("Subtitles: %1").arg(trackName(player.subtitleTracks[player.activeSubtitleTrack], player.activeSubtitleTrack));
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        visible: root.active
    }

    VideoOutput {
        id: video
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        visible: root.active
    }

    MediaPlayer {
        id: player
        videoOutput: video
        audioOutput: AudioOutput {
            id: audio
        }

        onErrorOccurred: (error, errorString) => {
            if (!root.active)
                return;
            root.error = errorString.length > 0 ? errorString : qsTr("Playback failed");
        }
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.BufferedMedia)
                root.retries = 0;
            // A live stream never ends on its own; reconnect a few times.
            if (mediaStatus === MediaPlayer.EndOfMedia && root.active) {
                if (root.retries < 3) {
                    root.retries++;
                    reconnect.restart();
                } else {
                    root.error = qsTr("The stream ended");
                }
            }
        }
    }

    Timer {
        id: reconnect
        interval: 1500
        onTriggered: root.retry()
    }

    Connections {
        target: Tvh
        function onStreamReady(uuid, url) {
            if (uuid !== root.channelUuid)
                return;
            root.awaitingUrl = false;
            player.source = url;
            player.play();
        }
        function onStreamFailed(uuid, message) {
            if (uuid !== root.channelUuid)
                return;
            root.awaitingUrl = false;
            root.error = message;
        }
    }
}
