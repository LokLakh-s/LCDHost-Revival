#!/usr/bin/env bash
# Builds the LCDHost AppImage from a built LCDHost.app, with linuxdeploy and
# its Qt plugin at pinned versions (checked by SHA-256). Qt, libusb and hidapi
# are bundled; the udev rules are built into LCDHost, which installs them
# from the settings of the Logitech plugins.
# Usage: packaging/linux/build-appimage.sh <LCDHost.app> <version> <output dir>
# Needs qmake of the Qt used for the build in PATH (or QMAKE set).
set -euo pipefail

app=$(cd "$1" && pwd)
version=$2
mkdir -p "$3"
out=$(cd "$3" && pwd)
src=$(cd "$(dirname "$0")/../.." && pwd)

LINUXDEPLOY_URL=https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage
LINUXDEPLOY_SHA256=c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d
PLUGIN_QT_URL=https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/1-alpha-20250213-1/linuxdeploy-plugin-qt-x86_64.AppImage
PLUGIN_QT_SHA256=15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

fetch() {
    local url=$1 sha256=$2 file="$work/tools/$(basename "$1")"
    mkdir -p "$work/tools"
    curl -sSfL -o "$file" "$url"
    echo "$sha256  $file" | sha256sum -c --quiet -
    chmod +x "$file"
}
fetch "$LINUXDEPLOY_URL" "$LINUXDEPLOY_SHA256"
fetch "$PLUGIN_QT_URL" "$PLUGIN_QT_SHA256"

# LCDHost looks for its plugins and WebKitServer next to its executable, and
# for the bundled layouts in ../layouts.
appdir="$work/LCDHost.AppDir"
mkdir -p "$appdir/usr/bin" "$appdir/usr/share/applications" \
    "$appdir/usr/share/icons/hicolor/128x128/apps"
cp -a "$app/bin/." "$appdir/usr/bin/"
find "$appdir/usr/bin" \( -name '*.a' -o -name '*.prl' \) -delete
if [ -d "$app/layouts" ]; then
    cp -a "$app/layouts" "$appdir/usr/layouts"
else
    cp -a "$src/layouts" "$appdir/usr/layouts"
fi
cp "$src/packaging/linux/lcdhost.desktop" "$appdir/usr/share/applications/"
cp "$src/applications/LCDHost/images/lcdhost128.png" \
    "$appdir/usr/share/icons/hicolor/128x128/apps/lcdhost.png"

# The plugins are loaded at run time: deploy their dependencies too.
deps_only=()
for lib in "$appdir"/usr/bin/*.so; do
    deps_only+=(--deploy-deps-only "$lib")
done

export QMAKE=${QMAKE:-$(command -v qmake)}
export APPIMAGE_EXTRACT_AND_RUN=1 # no FUSE needed
export LDAI_OUTPUT="$out/LCDHost-$version-x86_64.AppImage"
export LINUXDEPLOY_OUTPUT_VERSION="$version"
(cd "$work" && "$work/tools/linuxdeploy-x86_64.AppImage" \
    --appdir "$appdir" \
    --executable "$appdir/usr/bin/LCDHost" \
    --executable "$appdir/usr/bin/WebKitServer" \
    "${deps_only[@]}" \
    --desktop-file "$appdir/usr/share/applications/lcdhost.desktop" \
    --icon-file "$appdir/usr/share/icons/hicolor/128x128/apps/lcdhost.png" \
    --plugin qt \
    --output appimage)
ls -l "$LDAI_OUTPUT"
