# Plasma TV

A fullscreen, remote-controllable TV client for [TVHeadend](https://tvheadend.org),
built with Qt 6, Qt Quick and Kirigami. It follows Plasma theming and uses
Breeze Dark by default.

* **Live TV**: channel list with logos, the current programme and its progress,
  plus a detail panel with "up next"
* **TV guide**: timeline EPG grid with genre colours, programme details,
  scheduling/cancelling recordings
* **Watching**: zap with up/down, channel list overlay, info bar, audio-track
  and subtitle switching, numeric channel entry
* **Settings**: reachable from the top bar or with the MENU key; credentials
  go into the system wallet (KWallet / Secret Service via QtKeychain)
* **EPG cache**: SQLite (`~/.local/share/plasma-tv/epg-cache.sqlite`),
  so the guide is shown instantly and also works while offline

## Build

Supported on **Ubuntu** (26.04 and later) and **Arch Linux**, on both x86_64
and ARM64 (aarch64, including Arch Linux ARM). The code has no
architecture-specific parts, so build natively on the target machine.

### Arch Linux / Arch Linux ARM

Build and install a proper package with the bundled PKGBUILD:

```sh
sudo pacman -Syu --needed base-devel git   # toolchain (gcc, binutils, make)
cd packaging/arch
makepkg -si        # installs the dependencies, builds and installs the package
```

The PKGBUILD builds the committed state of this git repository. For a
development build, install the dependencies and use CMake directly:

```sh
sudo pacman -S --needed base-devel cmake git qt6-base qt6-declarative qt6-multimedia \
    qt6-multimedia-ffmpeg qt6-svg qtkeychain-qt6 kirigami qqc2-desktop-style breeze breeze-icons
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
```

If CMake reports that the C++ compiler "is not able to compile a simple test
program", the toolchain itself is broken, usually because of a partial upgrade
(`gcc`/`glibc`/`binutils` out of sync). Run `sudo pacman -Syu`, delete the
build directory and configure again.

### Ubuntu

```sh
sudo apt install cmake g++ qt6-base-dev qt6-declarative-dev qt6-declarative-dev-tools \
    qt6-multimedia-dev qtkeychain-qt6-dev libqt6sql6-sqlite \
    qml6-module-qtmultimedia qml6-module-qtquick-controls \
    qml6-module-org-kde-kirigami qml6-module-org-kde-desktop kde-style-breeze qt6-svg-plugins
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
sudo cmake --install build   # optional: installs to /usr/local with a launcher entry
```

### Running

```sh
./build/plasma-tv            # fullscreen
./build/plasma-tv --windowed # for development (F11 toggles)
```

Kirigami, the Plasma Qt Quick Controls style and Breeze (style and color
schemes) are used at runtime only, so no KF6 `-dev` packages are needed.

### Video decoding on ARM

Qt Multimedia decodes through FFmpeg. On many ARM boards it runs in software
because hardware decoding there usually goes through V4L2, which Qt Multimedia
doesn't use. Strong SoCs (e.g. RK3588) handle HD H.264 fine. The Raspberry Pi 5
has no H.264 decoder at all, and weaker boards may stutter on 1080i HD. In that
case create a transcoding profile in TVHeadend (e.g. H.264 720p) and enter its
name under *Settings → Streaming profile*.

## Remote control

| Key (remote / keyboard)                | Action                                      |
|----------------------------------------|---------------------------------------------|
| Arrows                                 | Navigate                                     |
| OK / Enter                             | Select, watch, open programme details       |
| Back / Esc / Backspace                 | Go back (from the top bar: return to video) |
| Menu / Settings / F2                   | Settings                                    |
| Guide / G / F3                         | TV guide                                    |
| 0-9                                    | Jump to channel number                      |
| Ch+/Ch- / PgUp/PgDn                    | Zap (watching) or page through lists        |
| ⏭ / ⏮ (or F / R) in the guide           | +/- 24 hours                                 |
| Info / I                               | Info bar while watching                     |
| Audio / A, Subtitle / S                | Cycle audio track / subtitles               |
| Stop / X                               | Stop playback                               |

Remotes using HDMI-CEC (kernel CEC input) or LIRC/ir-keytable mapped to these
key codes work out of the box.

## On-screen keyboard

Text entry uses regular input fields, and the app asks the input method to show
the Plasma virtual keyboard:

1. Install `plasma-keyboard` (`sudo apt install plasma-keyboard` / `sudo pacman -S plasma-keyboard`)
2. System Settings → Keyboard → Virtual Keyboard → *Plasma Keyboard*
3. Use a **Plasma Wayland** session (virtual keyboards are a Wayland feature).

KWin normally shows the virtual keyboard only after touch input. If it doesn't
appear when using a remote, set `KWIN_IM_SHOW_ALWAYS=1` in KWin's environment
(e.g. in `~/.config/plasma-workspace/env/`).

## Notes

* Streams use a short-lived TVHeadend ticket URL, so the password never ends up
  in the media URL. Very old servers without ticket support fall back to
  credentials in the URL.
* The streaming profile defaults to `pass`. Interlaced DVB channels are shown
  without deinterlacing by Qt Multimedia. A TVHeadend transcoding profile that
  deinterlaces works around that.
* Without an unlocked wallet, the password is kept for the current session only;
  the settings page shows a warning in that case.
* `-DPLASMA_TV_DEV_TOOLS=ON` builds a scripted UI driver (`PLASMA_TV_SCRIPT=file`) that
  is used for automated screenshots.
