# LCDHost Revival

[![build](https://github.com/LokLakh-s/LCDHost-Revival/actions/workflows/build.yml/badge.svg)](https://github.com/LokLakh-s/LCDHost-Revival/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](COPYING)

LCDHost is a compositing plugin manager for secondary displays. It renders
layouts (clocks, system monitors, media players, graphs, images…) and sends
them to small screens, originally the LCDs of Logitech G-series keyboards
(G15, G13, G510, G19).

It talks to these devices directly over USB/HID, so it can replace Logitech
Gaming Software and G HUB for the LCD. The goal of this revival is a fully
free driver for them, see [docs/HARDWARE.md](docs/HARDWARE.md).

This repository revives the project, which its original author
[Johan Lindh](https://github.com/linkdata/LCDHost) stopped maintaining in 2016.
The application itself is still called LCDHost.
The goal is to keep existing layouts working while bringing the code up to
date. See the [roadmap](ROADMAP.md).

> **Status:** early revival. The application builds and runs on Windows and
> Linux with Qt 6. Real hardware has not been re-tested yet: reports from
> G15/G19 owners are very welcome.

## Features

- Layout editor with live preview, drag and drop, and per-element settings.
- Plugins for text, dials, bars, graphs, images, decorations, cursors and
  menus, logic, weather (from [Open-Meteo](https://open-meteo.com/)), system monitoring (AIDA64, MSI Afterburner, CoreTemp, GPU-Z,
  HWiNFO, SpeedFan), now playing (iTunes, Winamp, foobar2000, VLC, Spotify),
  TeamSpeak 3, web pages and RSS feeds.
- Output drivers for Logitech 160×43 and 320×240 LCDs (direct HID/USB or
  through the Logitech LCD SDK) and a **virtual LCD** for testing without
  hardware.

## Getting LCDHost

Download the latest version for Windows x64 or Linux x64 from the
[releases page](https://github.com/LokLakh-s/LCDHost-Revival/releases); the
release notes explain what each archive needs. Builds of every `master`
commit are also available from the
[build runs](https://github.com/LokLakh-s/LCDHost-Revival/actions/workflows/build.yml)
(`LCDHost-windows-x64` and `LCDHost-linux-x64`, GitHub sign-in required).

On first start:

1. Plugins are disabled by default. Enable the ones you need in the
   **Plugins** tab (select a plugin, then **Load**).
2. Pick an output device in the **Output** tab. Without hardware, use one of
   the *Virtual* devices provided by the VirtualLCD plugin.
3. The bundled layouts are copied to `Documents/LCDHost/layouts/` the first
   time LCDHost starts.

## Building from source

Requirements: Qt **6.8 or later** (the CI uses 6.11) with the `qtwebengine`,
`qtwebchannel` and `qtpositioning` modules for the WebKit plugin, a C++17
compiler, [libusb](https://libusb.info/) 1.0 and
[hidapi](https://github.com/libusb/hidapi). Install Qt with the
[Qt online installer](https://www.qt.io/download-qt-installer-oss) or
[aqtinstall](https://github.com/miurahr/aqtinstall), and put its `bin`
directory in your `PATH`.

### Linux

libusb and hidapi come from the distribution and are found with
`pkg-config` (`libusb-1.0` and `hidapi-hidraw`).

```sh
sudo apt-get install build-essential pkg-config libusb-1.0-0-dev libhidapi-dev libgl1-mesa-dev libxkbcommon-dev
mkdir build && cd build
qmake ../LCDHost.pro -r
make -j"$(nproc)"
cp -r ../layouts LCDHost.app/
./LCDHost.app/bin/LCDHost
```

### Windows

With Qt 6 (`msvc2022_64`), Visual Studio 2022 and 7-Zip, first fetch the
dependencies once.
[`packaging/windows/fetch-deps.ps1`](packaging/windows/fetch-deps.ps1)
downloads the official libusb and hidapi binaries at pinned versions and
checks their SHA-256:

```bat
pwsh packaging\windows\fetch-deps.ps1 -Destination C:\lcdhost-deps
```

It prints two variables: set the environment variables `LIBUSB_DIR` and
`HIDAPI_DIR` to these values. Then, from an "x64 Native Tools" command
prompt:

```bat
mkdir build && cd build
qmake ..\LCDHost.pro -r
nmake
windeployqt --release --no-translations LCDHost.app\bin\LCDHost.exe
windeployqt --release --no-translations LCDHost.app\bin\WebKitServer.exe
xcopy /E /I ..\layouts LCDHost.app\layouts
```

### Smoke test

`tests/smoke/run.sh` starts LCDHost headless with the default layout and
checks that it does not crash and that the core plugins and the Logitech
drivers load:

```sh
tests/smoke/run.sh build/LCDHost.app/bin 20
```

## Documentation

- [Hardware setup](docs/HARDWARE.md): drivers and permissions for each
  supported device and operating system.
- [Architecture overview](docs/ARCHITECTURE.md): how the host, plugins,
  layouts and devices fit together.
- [Contributing](CONTRIBUTING.md): how to report bugs, propose changes and
  submit pull requests.
- [Roadmap](ROADMAP.md) and [changelog](CHANGELOG.md).
- [Third-party components](docs/THIRD_PARTY.md) bundled in this repository.
- [Security policy](SECURITY.md).

## License

LCDHost is free software, released under the
[GNU General Public License v3](COPYING) or later. The plugin API header
(`linkdata/lh_api5plugin/lh_plugin.h`) is BSD-licensed so that plugins can
use any license. Bundled third-party code keeps its own license, see
[docs/THIRD_PARTY.md](docs/THIRD_PARTY.md).

Copyright © 2009–2016 Johan Lindh (Link Data Stockholm), Andy Bridges
(CodeLeap) and contributors.
