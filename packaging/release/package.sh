#!/usr/bin/env bash
# Builds the release archives, their checksums and the release notes from the
# build artifacts of the CI.
# Usage: packaging/release/package.sh <version> <artifacts dir> <output dir>
#   <artifacts dir> holds the LCDHost-linux-x64 and LCDHost-windows-x64
#   folders downloaded from the build jobs.
set -euo pipefail

version=$1
artifacts=$(cd "$2" && pwd)
mkdir -p "$3"
out=$(cd "$3" && pwd)
src=$(cd "$(dirname "$0")/../.." && pwd)

source_version=$(sed -n 's/^ *LH_VERSION=//p' "$src/qmakecache/qmakecache.pro")
if [ "$source_version" != "$version" ]; then
    echo "error: tag version $version does not match LH_VERSION $source_version" >&2
    exit 1
fi

# The section of the version, with its wrapped list items joined into one
# line each: GitHub shows line breaks of release notes as is.
notes=$(awk -v header="## [$version]" '
    index($0, header) == 1 { found = 1; next }
    found && (/^## \[/ || /^\[[^]]+\]: /) { exit }
    found && !started && /^[[:space:]]*$/ { next }
    found && started && /^  +[^ ]/ && line != "" { sub(/^ +/, ""); line = line " " $0; next }
    found { if (started) print line; started = 1; sub(/^### /, "## "); line = $0 }
    END { if (started) print line }
' "$src/CHANGELOG.md")
if [ -z "$(echo "$notes" | tr -d '[:space:]')" ]; then
    echo "error: no \"## [$version]\" section in CHANGELOG.md" >&2
    exit 1
fi

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT

linux_dir="LCDHost-$version-linux"
cp -a "$artifacts/LCDHost-linux-x64" "$stage/$linux_dir"
# Artifact downloads do not keep the executable bit.
chmod +x "$stage/$linux_dir/bin/LCDHost" "$stage/$linux_dir/bin/WebKitServer"
tar -C "$stage" --owner=0 --group=0 -czf "$out/LCDHost-$version-linux-x64.tar.gz" "$linux_dir"

windows_dir="LCDHost-$version-windows"
cp -a "$artifacts/LCDHost-windows-x64" "$stage/$windows_dir"
find "$stage/$windows_dir" \( -name '*.lib' -o -name '*.exp' \) -delete
(cd "$stage" && zip -qr9 "$out/LCDHost-$version-windows-x64.zip" "$windows_dir")

(cd "$out" && sha256sum "LCDHost-$version-linux-x64.tar.gz" "LCDHost-$version-windows-x64.zip" > SHA256SUMS)

{
    sed "s/@VERSION@/$version/g" "$src/packaging/release/notes-header.md"
    echo
    echo "$notes"
} > "$out/notes.md"

echo "Release files in $out:"
ls -l "$out"
