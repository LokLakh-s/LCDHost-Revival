#!/usr/bin/env bash
# Headless smoke test: start LCDHost with the default layout and the core
# plugins enabled, then check that it neither crashes nor fails to load them.
# Usage: tests/smoke/run.sh <path to LCDHost.app/bin> [seconds]
set -u

BIN_DIR=$(cd "$1" && pwd)
DURATION=${2:-20}
SRC_DIR=$(cd "$(dirname "$0")/../.." && pwd)
# The Logitech drivers must load without any device connected; they check
# that libusb and hidapi are found.
PLUGINS="Bar Decor Dial Graph Image Text VirtualLCD Lg160x43 Lg320x240 LgBacklight"

export HOME=$(mktemp -d)
export XDG_CONFIG_HOME="$HOME/.config"
export QT_QPA_PLATFORM=offscreen
mkdir -p "$HOME/Documents/LCDHost" "$HOME/.config/Link Data"
# Bundled layouts (next to bin/) are installed by LCDHost itself on first run.
if [ ! -d "$BIN_DIR/../layouts" ]; then
    cp -r "$SRC_DIR/layouts" "$HOME/Documents/LCDHost/"
fi
{
    echo '[plugins]'
    for p in $PLUGINS; do echo "libLH_$p.so.1.0\\enabled=true"; done
} > "$HOME/.config/Link Data/LCDHost.conf"

LOG="$HOME/lcdhost.log"
# Run from an unrelated directory so library lookup cannot rely on the cwd.
(cd / && timeout "$DURATION" "$BIN_DIR/LCDHost" > "$LOG" 2>&1)
status=$?

failed=0
if [ "$status" -ne 124 ]; then
    echo "FAIL: LCDHost exited early with status $status"
    failed=1
fi
for p in $PLUGINS; do
    if ! grep -q "\"LH_$p\" loaded with" "$LOG"; then
        echo "FAIL: plugin LH_$p did not load"
        failed=1
    fi
done
if [ ! -f "$HOME/Documents/LCDHost/layouts/g19-default/g19-default.xml" ]; then
    echo "FAIL: default layout not installed"
    failed=1
fi
if ! grep -q "Loading</span> Complete" "$LOG"; then
    echo "FAIL: default layout did not load"
    failed=1
fi
if [ "$failed" -ne 0 ]; then
    echo "--- relevant log lines ---"
    grep -vE 'nbsp|setParent|propagateSizeHints' "$LOG" | head -n 80
    exit 1
fi
echo "OK: LCDHost ran ${DURATION}s and loaded: $PLUGINS"
