> Hardware support has not been confirmed on real devices yet. If you own a G13, G15, G19, G510 or Z-10, a [hardware test report](https://github.com/LokLakh-s/LCDHost-Revival/issues/new/choose) helps a lot.

## Downloads

- **Windows 10/11 (x64)**: `LCDHost-@VERSION@-windows-x64.zip`. Unzip it anywhere and run `bin\LCDHost.exe`. The Qt runtime, libusb and hidapi are included; if Windows reports a missing `MSVCP140.dll` or `VCRUNTIME140.dll`, run `bin\vc_redist.x64.exe` once.
- **Linux (x64), AppImage**: `LCDHost-@VERSION@-x86_64.AppImage`. Make it executable (`chmod +x`) and run it; if your system lacks FUSE, run it with `--appimage-extract-and-run`. Qt and hidapi are included; libusb 1.0 comes from your system, as AppImages expect (package `libusb-1.0-0` on Debian and Ubuntu, `libusb1` on Fedora). For the Logitech devices, click *Install udev rules* in the settings of a Logitech plugin.
- **Linux (x64), archive**: `LCDHost-@VERSION@-linux-x64.tar.gz`. Extract it and run `bin/LCDHost`. Qt is **not** bundled: install Qt 6.11 or later (Core, Gui, Widgets, Network, Xml and WebEngine) from your distribution or the Qt installer, and the libusb 1.0 and hidapi (hidraw) libraries from your distribution.
- `SHA256SUMS`: checksums of the files above (`sha256sum -c SHA256SUMS`).

Logitech LCDs need a one-time setup: see [docs/HARDWARE.md](https://github.com/LokLakh-s/LCDHost-Revival/blob/v@VERSION@/docs/HARDWARE.md).
