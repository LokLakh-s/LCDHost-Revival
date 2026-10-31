# Contributing to LCDHost

Thanks for your interest in LCDHost! This project is a revival of an
application abandoned in 2016, so help of any kind is valuable: bug reports,
hardware testing, layouts, documentation and code.

By participating you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Ways to help

- **Test on real hardware.** The maintainers currently have no Logitech LCD
  keyboard. If you own a G13, G15, G510, G19 or Z10, telling us what works and
  what does not is one of the most useful contributions.
- **Report bugs** and **suggest features** through
  [issues](https://github.com/LokLakh-s/LCDHost-Revival/issues/new/choose).
- **Pick an item from the [roadmap](ROADMAP.md)**, or an issue labelled
  `good first issue` or `help wanted`.
- **Improve the documentation.**

## Reporting a bug

Search the existing issues first. When opening a new one, use the bug report
form and include:

- LCDHost version (Help → About) and where you got it (CI build, own build).
- Operating system and version.
- Output device (model, or "virtual") and, on Windows, whether Logitech Gaming
  Software or G HUB is installed.
- Steps to reproduce, what you expected and what happened.
- The log file if possible: LCDHost writes one per run to
  `Documents/LCDHost/logs/`. Remove anything personal from it before
  attaching it.

Please do **not** report security vulnerabilities in public issues, see
[SECURITY.md](SECURITY.md).

## Development setup

See [Building from source](README.md#building-from-source). In short: Qt 6
(6.8 or later) with `qtwebengine`, then `qmake ../LCDHost.pro -r` and `make` (or `nmake` on
Windows) from a separate build directory.

Useful tips:

- Use the **VirtualLCD** plugin as output device to work without hardware.
- `tests/smoke/run.sh <build>/LCDHost.app/bin` runs the headless smoke test
  used by the CI.
- On Linux, `QT_QPA_PLATFORM=offscreen` runs LCDHost without a display.

Read the [architecture overview](docs/ARCHITECTURE.md) before making larger
changes.

## Submitting changes

1. Fork the repository and create a branch from `master`.
2. Keep each pull request focused on one topic. Separate refactoring from
   behaviour changes.
3. Make sure the project builds and the smoke test passes. The CI builds on
   Linux and Windows for every pull request.
4. Update the documentation and add an entry to the `Unreleased` section of
   [CHANGELOG.md](CHANGELOG.md) for user-visible changes.
5. Open the pull request and fill in the template.

### Coding guidelines

- Follow the style of the surrounding code (indentation, naming, brace
  placement). Do not reformat code you are not otherwise changing.
- Code, comments, commit messages and documentation are written in English.
- Comments explain *why*, not *what*. Keep them short and accurate.
- Keep plugin binary compatibility in mind: the plugin API lives in
  `linkdata/lh_api5plugin/lh_plugin.h`. Changing it affects every plugin.
- Existing layouts must keep loading. When changing or removing a setting,
  think about layouts saved by older versions.
- Do not commit build outputs, generated files, IDE settings or secrets
  (API keys, tokens).

### Commit messages

- Short summary line in the imperative mood ("Fix crash when…", not "Fixed"),
  ideally under 72 characters.
- A blank line, then a body explaining what changed and why, wrapped at 72
  characters.

## Releases

Maintainers release from `master`:

1. Move the `Unreleased` entries of [CHANGELOG.md](CHANGELOG.md) to a new
   `## [x.y.z] - YYYY-MM-DD` section and update the links at the bottom.
2. Set `LH_VERSION` in `qmakecache/qmakecache.pro` and update
   `applications/LCDHost/whatsnew.html`.
3. Commit ("Release x.y.z"), tag the commit `vx.y.z`, then push the commit
   and the tag. The CI builds both platforms and publishes the GitHub
   release: the archives, the AppImage, `SHA256SUMS`, and the changelog
   section as notes (see `packaging/release/`). It refuses a tag that does
   not match `LH_VERSION` or has no changelog section.

## Licensing of contributions

LCDHost is licensed under the [GNU GPL v3 or later](COPYING). By submitting a
contribution you agree that it is distributed under the same license. Plugin
API headers are BSD-licensed; keep contributions to them under that license.

Only contribute code you have the right to contribute. Do not add third-party
code or SDKs without checking that their license allows redistribution, and
list them in [docs/THIRD_PARTY.md](docs/THIRD_PARTY.md).
