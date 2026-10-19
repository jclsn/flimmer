// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtMultimedia
import PlasmaTv

// Video layer. Lives outside the scaled UI so it always renders at native resolution.
Item {
    id: root
    property string channelUuid
    property string error
    property bool awaitingUrl: false
    // Reconnect attempts for the current channel before an error is shown.
    property int retries: 0
    readonly property int maxRetries: 3
    readonly property bool active: channelUuid.length > 0
    readonly property alias mediaPlayer: player

    // Pausing records the channel on the server; resuming plays that recording, so
    // the programme continues where it was paused.
    property bool paused: false
    property string timeshiftUuid   // DVR entry of the recording
    property bool timeshifting: false   // playing the recording instead of live TV
    property double pausedAt: 0
    property real pendingSeek: 0
    readonly property real behindLive: timeshifting || paused ? Math.max(0, Tvh.now - pausedAt - (timeshifting ? player.position : 0)) : 0

    function togglePause() {
        if (!active || awaitingUrl)
            return;

        if (!paused) {
            player.pause();
            paused = true;

            if (!timeshifting && timeshiftUuid.length === 0) {
                pausedAt = Date.now();
                Tvh.startTimeshift(channelUuid);
            }

            return;
        }

        paused = false;

        // Without a recording (yet), the paused live stream simply continues.
        if (!timeshifting && timeshiftUuid.length > 0)
            openRecording(0);
        else
            player.play();
    }

    function openRecording(position) {
        pendingSeek = position;
        awaitingUrl = true;
        Tvh.requestRecording(timeshiftUuid);
    }

    function endTimeshift() {
        Tvh.removeTimeshift(timeshiftUuid);
        timeshiftUuid = "";
        timeshifting = false;
        paused = false;
        pendingSeek = 0;
    }

    function play(uuid) {
        if (uuid !== channelUuid)
            retries = 0;

        endTimeshift();
        reconnect.stop();
        player.stop();
        player.source = "";
        channelUuid = uuid;
        error = "";
        awaitingUrl = true;
        Tvh.requestStream(uuid);
    }

    // Manual retry from the error screen gets a fresh set of reconnect attempts.
    function retry() {
        retries = 0;
        reconnectNow();
    }

    function reconnectNow() {
        if (channelUuid.length > 0)
            play(channelUuid);
    }

    function scheduleReconnect(message) {
        if (retries < maxRetries) {
            retries++;
            reconnect.restart();
        } else {
            error = message;
        }
    }

    function stop() {
        endTimeshift();
        reconnect.stop();
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
            // Errors while a new stream is being requested belong to the cancelled one, and
            // the server may drop a paused live stream.
            if (!root.active || root.awaitingUrl || root.paused)
                return;
            // IPTV sources and busy tuners often fail once; reconnect with a fresh ticket.
            root.scheduleReconnect(errorString.length > 0 ? errorString : qsTr("Playback failed"));
        }
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.BufferedMedia)
                root.retries = 0;

            if (root.pendingSeek > 0 && (mediaStatus === MediaPlayer.LoadedMedia || mediaStatus === MediaPlayer.BufferedMedia)) {
                player.position = root.pendingSeek;
                root.pendingSeek = 0;
            }

            if (mediaStatus !== MediaPlayer.EndOfMedia || !root.active || root.paused)
                return;

            // The end of the recording so far: continue from the grown file, or with
            // live TV once playback has caught up.
            if (root.timeshifting) {
                if (Date.now() - root.pausedAt - player.position < 15000)
                    root.play(root.channelUuid);
                else
                    root.openRecording(player.position);

                return;
            }

            // A live stream never ends on its own; reconnect a few times.
            root.scheduleReconnect(qsTr("The stream ended"));
        }
    }

    Timer {
        id: reconnect
        interval: 1500
        onTriggered: root.reconnectNow()
    }

    Connections {
        target: Tvh
        function onStreamReady(uuid, url) {
            if (uuid === root.timeshiftUuid && uuid.length > 0) {
                root.awaitingUrl = false;
                root.timeshifting = true;
                // The URL may not change when the grown recording is reopened.
                player.source = "";
                player.source = url;
                player.play();

                return;
            }

            if (uuid !== root.channelUuid)
                return;
            root.awaitingUrl = false;
            player.source = url;
            player.play();
        }
        function onStreamFailed(uuid, message) {
            if (uuid === root.timeshiftUuid && uuid.length > 0) {
                // The recording can't be played; continue with live TV.
                root.play(root.channelUuid);

                return;
            }

            if (uuid !== root.channelUuid)
                return;
            root.awaitingUrl = false;
            root.error = message;
        }
        function onTimeshiftStarted(channelUuid, dvrUuid) {
            // Resumed (or switched channels) before the server answered.
            if (channelUuid !== root.channelUuid || !root.paused) {
                Tvh.removeTimeshift(dvrUuid);

                return;
            }

            root.timeshiftUuid = dvrUuid;
        }
    }
}
