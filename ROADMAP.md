# Roadmap

This roadmap describes where the LCDHost revival is heading. It is a plan,
not a promise: priorities can change, and contributions on any item are
welcome (open an issue first for larger items so that work is not
duplicated).

Legend: ✅ done · 🚧 in progress · ⬜ planned

## Phase 0: Revival (done)

- ✅ Build again on Windows (current MSVC and Windows SDK) and Linux with Qt 5.15.
- ✅ Continuous integration on Linux and Windows, with a headless smoke test
  and downloadable builds.
- ✅ Fix startup crash and crashes when the selection is cleared.
- ✅ Fix plugin loading on Linux and install the bundled layouts on first run.
- ✅ Remove the insecure online update mechanism (unsigned downloads over HTTP).
- ✅ Remove integrations with discontinued software (Fraps, classic RivaTuner,
  EVEREST, ATITrayTools, Sidebar gadgets, online lyrics services).
- ✅ Contributor documentation, issue templates and this roadmap.

## Phase 1: Modern toolchain

- ✅ **Port to Qt 6** (6.8 or later) and drop Qt 4/5 code paths. CI builds
  with Qt 6.11 on Linux and Qt 6.10 on Windows, until aqtinstall supports the
  new Qt 6.11+ repository layout for Windows.
- ⬜ Replace the remaining deprecated Qt APIs reported by the compiler, and
  remove (or port to `QOpenGLWidget`) the OpenGL rendering path, which has
  been disabled at build time since the original project.
- ⬜ **Move the build from qmake to CMake**, the build system recommended for
  Qt 6, and remove the custom qmake feature machinery.
- 🚧 **Replace outdated bundled libraries** with current upstream versions,
  obtained through the system package manager on Linux and at pinned,
  checksummed versions on Windows:
  - ✅ libusbx 1.0.15 → libusb, signal11 HIDAPI → libusb/hidapi.
  - ⬜ TagLib 1.7 → TagLib 2, and drop the bundled zlib 1.2.5 (known
    vulnerabilities).
- ⬜ Replace in-house helpers with Qt equivalents: `codeleap/json` →
  `QJsonDocument`.
- ⬜ DataViewer: stop using a layout-provided string as a `printf` format.
- ⬜ **Stop redistributing proprietary SDKs** (Logitech LCD SDK, iTunes COM
  SDK): let developers point the build to their own copy and make the
  dependent features optional.
- ⬜ Target Windows 10 or later (required by Qt 6) and remove Windows XP era
  workarounds.
- ⬜ Clean up compiler warnings and keep the build warning-free in CI.

## Phase 2: Repair broken features

- ✅ **Weather**: replace the defunct Yahoo Weather API with
  [Open-Meteo](https://open-meteo.com/) (free, no API key).
- ⬜ **RSS and web pages** (WebKit plugin): follow every kind of HTTP
  redirect, report network errors, resolve relative URLs, and make page
  capture reliable with Qt WebEngine (animated and JavaScript-driven pages).
- ⬜ **Now Playing**: use the operating system media APIs (Windows System
  Media Transport Controls, MPRIS on Linux) so that any modern player works,
  instead of per-player window hacks that broke over time (Spotify, iTunes /
  Apple Music). Support the VLC web interface password. Check whether the
  "MSN Now Playing" message receiver is still useful.
- ⬜ **TeamSpeak**: send the ClientQuery API key required since TeamSpeak
  3.1.3, and investigate TeamSpeak 5/6.
- ⬜ **Monitoring**: support the current HWiNFO shared memory interface
  (`HWiNFO_SENS_SM2`) and re-check the other sources (AIDA64, MSI Afterburner,
  CoreTemp, GPU-Z, SpeedFan).
- ⬜ **DataViewer**: support 64-bit processes.
- ⬜ Re-check Mailcount and DriveStats on current Windows versions.

## Phase 3: A free driver for Logitech LCDs

Goal: LCDHost works as a complete FOSS alternative to Logitech Gaming Software
and G HUB for the LCD keyboards and speakers, with no proprietary component.
See [docs/HARDWARE.md](docs/HARDWARE.md) for the current setup steps.

- ✅ Document device setup per operating system and ship udev rules for Linux
  (`packaging/linux/70-lcdhost.rules`).
- ✅ **Windows: install the G19 driver from LCDHost.** *Install driver* and
  *Remove driver* in the settings of the G19 plugin run
  `LCDHostDriverSetup.exe`, an elevated helper built on
  [libwdi](https://github.com/pbatard/libwdi) (LGPL v3) that binds the inbox
  WinUSB driver to the G19 display interface only (`MI_00`), and removes the
  driver package and its certificate again. The CI installs and removes it
  on every Windows build; it still has to be confirmed with a real G19.
- ✅ **Use upstream libusb and hidapi**: system packages on Linux
  (pkg-config), official binaries at pinned versions on Windows, instead of
  the 2013-era copies.
- ⬜ G19: backlight colour through the HID interface (hidraw on Linux), and
  G-keys / M-keys as layout inputs.
- ⬜ Make the proprietary Logitech LCD SDK plugin (`LgLcdMan`) optional and
  off by default; keep it only for users who want to stay on LGS.
- 🚧 Clear messages when the device is busy (LGS or G HUB running) or the
  driver or permissions are missing: done for the G19 (status in the plugin
  settings), still to do for the HID devices.
- ⬜ Linux packages (AppImage or Flatpak, and distribution packages) that
  install the udev rules.

## Phase 4: Hardware and platforms

- ⬜ **Hardware validation** with G13, G15, G19, G510 and Z10 owners (see the
  hardware test report issue form).
- ⬜ **Generic displays**: output drivers for non-Logitech screens, such as
  small USB or serial displays driven by an ESP32 or a Raspberry Pi, or frame
  streaming over the network.
- ⬜ Decide on macOS: the code has macOS support that nobody maintains or
  tests at the moment.

## Phase 5: Releases and quality

- ✅ Releases built and published by the CI from a version tag, with the
  changelog as release notes and SHA-256 checksums.
- ⬜ A Windows installer.
- ⬜ An optional, safe update notifier: check GitHub releases over HTTPS and
  tell the user, never download or run code automatically.
- ⬜ Automated tests for layout loading and saving, setup items and plugin
  loading.
- ⬜ Ask for confirmation before a layout launches a program (Cursor plugin
  actions).
- ⬜ User documentation: layout editor guide, plugin reference.
- ⬜ Translations of the user interface.
