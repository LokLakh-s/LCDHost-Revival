# Changelog

All notable changes to this project are documented in this file. The format
is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

Versions up to 0.0.38 were released by the original author; see the
[original repository](https://github.com/linkdata/LCDHost).

## [Unreleased]

### Added

- Releases: pushing a version tag makes the CI publish a GitHub release
  with the Windows and Linux archives, SHA-256 checksums and the
  changelog section as notes.

### Removed

- A Qt installer log committed by mistake, and the linker's `.lib`/`.exp`
  files from the Windows build.

## [0.1.0] - 2026-09-27

### Changed

- LCDHost now requires Qt 6 (6.8 or later). Qt 4 and Qt 5 code paths are gone.
- Timers use `QElapsedTimer` and regular expressions use
  `QRegularExpression`.

- Weather: data now comes from Open-Meteo (free, no API key) instead of the
  defunct Yahoo! Weather API. Locations are looked up by name ("Paris,
  France") or given as "latitude, longitude". Existing layouts and image
  maps keep working: conditions are still reported with the former Yahoo!
  weather codes.

### Added

- Hardware setup guide (docs/HARDWARE.md) and udev rules for Linux
  (packaging/linux/70-lcdhost.rules).

### Fixed

- Weather and DataViewer image maps and DataViewer data files failed to
  load when they used LF line endings, which is the case of the bundled
  layouts since 0.0.41.
- Cursor plugin: adding a secondary cursor after "Secondary Cursor [n]"
  always produced "[2]".
- LCoreReboot: possible read past the end of the process path when
  translating device paths to drive letters.
- Monitoring (Core Temp): the CPU name is read within the bounds of the
  shared-memory field.

### Removed

- The bundled Qt model test and an unused qmake include file.

## [0.0.42] - 2026-09-27

### Changed

- The repository moved to [LokLakh-s/LCDHost-Revival](https://github.com/LokLakh-s/LCDHost-Revival);
  links in the application and documentation point to it.

## [0.0.41] - 2026-09-27

### Removed

- The automatic update mechanism. It downloaded plugins and installers over
  plain HTTP without verifying them, and its server no longer exists.
- Monitoring sources for discontinued software: Fraps, classic RivaTuner (and
  the RivaTuner shared memory writer), ATITrayTools, HWMonitor and Logitech
  battery Sidebar gadgets, and the EVEREST fallback of the AIDA64 source.
- Online lyrics lookup in Now Playing (LyricWiki, LyrDB and Letras are gone).
  The `{lyrics}` token is kept but stays empty.
- Unused sources, build outputs and obsolete scripts.

### Changed

- The About and Welcome dialogs point to the GitHub repository instead of
  the defunct forum.
- Line endings are normalized to LF.

### Added

- Contributor documentation, code of conduct, security policy, issue and
  pull request templates, roadmap and architecture overview.

## [0.0.40] - 2026-09-27

### Fixed

- Windows builds with current MSVC and Windows SDK (Qt 4 API leftovers,
  struct packing around Windows headers, missing `user32` link).
- The fallback to the default layout directory looked for the wrong path.

### Added

- Bundled layouts are installed on first run.
- Windows CI build with the Qt runtime and layouts included, and a startup test.

## [0.0.39] - 2026-09-27

### Fixed

- Crash at startup and when the class or instance selection is cleared.
- Linux: plugins now load regardless of the working directory.

### Added

- Plugin load failures are logged.
- Build instructions, headless smoke test and Linux CI.

[Unreleased]: https://github.com/LokLakh-s/LCDHost-Revival/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/LokLakh-s/LCDHost-Revival/releases/tag/v0.1.0
[0.0.42]: https://github.com/LokLakh-s/LCDHost-Revival/releases/tag/v0.0.42
[0.0.41]: https://github.com/LokLakh-s/LCDHost-Revival/releases/tag/v0.0.41
[0.0.40]: https://github.com/LokLakh-s/LCDHost-Revival/releases/tag/v0.0.40
[0.0.39]: https://github.com/LokLakh-s/LCDHost-Revival/releases/tag/v0.0.39
