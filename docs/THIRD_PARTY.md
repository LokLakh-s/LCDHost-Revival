# Third-party components

LCDHost is licensed under the GNU GPL v3 or later. This repository also
contains code from other projects, which keeps its own license. The table
below lists what is bundled, where, and under which terms.

| Component | Version | Location | License | Used by |
|---|---|---|---|---|
| [TagLib](https://taglib.org/) | 1.7.0 | `3rdParty/taglib/` | LGPL 2.1 or MPL 1.1 | `LH_NowPlaying` (cover art, tags) |
| [zlib](https://zlib.net/) | 1.2.5 | `3rdParty/zlib/` | zlib license | TagLib |
| Logitech LCD SDK (`lglcd.h`, `lglcd.lib`, `liblgLcd.a`) | 3.0 or later | `plugins/LH_LgLcdMan/win/`, `plugins/LH_LgLcdMan/mac/` | **Proprietary** (Logitech LCD SDK License Agreement) | `LH_LgLcdMan` |
| iTunes COM SDK (generated headers) | — | `plugins/LH_NowPlaying/SDKs/iTunes/` | **Proprietary** (Apple) | `LH_NowPlaying` |
| Winamp SDK headers (`wa_*.h`) | — | `plugins/LH_NowPlaying/SDKs/Winamp/` | Nullsoft SDK terms | `LH_NowPlaying` |
| Icons based on the Silk set (resized to 12, 14 and 16 px) | — | `layouts/g19-eos/Status Icons/` | [CC BY 2.5](https://creativecommons.org/licenses/by/2.5/), Silk by Mark James (famfamfam.com) | sample layout |

## External libraries

These libraries are not in the repository: the build finds them on the system
(Linux, macOS) or downloads them at pinned versions, checked by SHA-256, with
[`packaging/windows/fetch-deps.ps1`](../packaging/windows/fetch-deps.ps1)
(Windows). The Windows release ships them as listed.

| Component | Version on Windows | License | Used by | Shipped on Windows as |
|---|---|---|---|---|
| [libusb](https://libusb.info/) | 1.0.30 (official binaries) | LGPL 2.1 or later | `LH_Lg320x240` (G19 driver) | `libusb-1.0.dll` |
| [hidapi](https://github.com/libusb/hidapi) | 0.15.0 (official binaries) | GPL v3, BSD-style, or original HIDAPI license (at your choice) | `lh_hid`, Logitech HID drivers | `hidapi.dll` |

The sources of libusb and hidapi are available from their repositories at
the tags of these versions.

## Notes

- **Proprietary SDKs.** The Logitech LCD SDK and the iTunes COM SDK were
  shipped by the original project. Their license agreements are not included
  in this repository, and it has not been verified that they allow
  redistribution. Removing them from the repository (and asking developers to
  install the SDKs themselves) is tracked in the [roadmap](../ROADMAP.md).
- **Outdated copies.** The bundled TagLib and zlib are more than ten years
  old and miss security fixes (zlib 1.2.5 in particular). Replacing them with
  current upstream versions, as was done for libusb and hidapi, is part of
  the roadmap.
- **Sample layouts.** The artwork in `layouts/` comes from the original
  project and its community. If you hold rights on an image and want it
  removed or credited differently, please open an issue.

## Online services

| Service | Used by | Terms |
|---|---|---|
| [Open-Meteo](https://open-meteo.com/) forecast and geocoding APIs | `LH_Weather` | Free for non-commercial use (under 10,000 calls per day per user). Weather data by [Open-Meteo.com](https://open-meteo.com/), licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). |

If you add a third-party component or service, add it here in the same pull
request.
