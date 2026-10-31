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
- Free output drivers for the Logitech G13, G15, G19, G510 and Z-10 LCDs
  (direct HID/USB, no Logitech software needed), which also report the
  G-keys, M-keys and LCD buttons to layouts and set the keyboard backlight,
  and a **virtual LCD** for testing without hardware.

## Getting LCDHost

Download the latest version for Windows x64 or Linux x64 (an AppImage, or an
archive that uses the Qt of your system) from the
[releases page](https://github.com/LokLakh-s/LCDHost-Revival/releases); the
release notes explain what each file needs. Builds of every `master`
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

With Qt 6 (`msvc2022_64`) and Visual Studio 2022 (with the x64 and ARM64
C++ build tools), git and 7-Zip, first fetch the dependencies once.
[`packaging/windows/fetch-deps.ps1`](packaging/windows/fetch-deps.ps1)
downloads the official libusb and hidapi binaries at pinned versions, checks
their SHA-256, and builds [libwdi](https://github.com/pbatard/libwdi) for the
G19 driver installer:

```bat
pwsh packaging\windows\fetch-deps.ps1 -Destination C:\lcdhost-deps
```

It prints three variables: set the environment variables `LIBUSB_DIR`,
`HIDAPI_DIR` and `LIBWDI_DIR` to these values. Without `LIBWDI_DIR`, everything
except the G19 driver installer (`LCDHostDriverSetup.exe`) is built. Then,
from an "x64 Native Tools" command prompt:

```bat
mkdir build && cd build
qmake ..\LCDHost.pro -r
nmake
windeployqt --release --no-translations LCDHost.app\bin\LCDHost.exe
windeployqt --release --no-translations LCDHost.app\bin\WebKitServer.exe
xcopy /E /I ..\layouts LCDHost.app\layouts
```

The `LgLcdMan` plugin, which drives the LCDs through Logitech Gaming
Software, needs the proprietary Logitech LCD SDK, which this repository does
not include. To build it, set `LGLCD_SDK_DIR` to a folder that holds `lglcd.h`
and the x64 `lglcd.lib` of your copy of the SDK; otherwise it is skipped.

### Tests

`tests/smoke/run.sh` starts LCDHost headless with the default layout and
checks that it does not crash and that the core plugins and the Logitech
drivers load. `tests/hid/run-fake-devices.sh` runs it with simulated Logitech
HID devices (a fake hidapi) and checks what the drivers do with them, and
`tests/hid/tst_hid.pro` holds unit tests of the HID report parsing (Linux):

```sh
tests/smoke/run.sh build/LCDHost.app/bin 20
tests/hid/run-fake-devices.sh build/LCDHost.app/bin 15
mkdir build-tests && cd build-tests && qmake ../tests/hid/tst_hid.pro && make && ./tst_hid
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
