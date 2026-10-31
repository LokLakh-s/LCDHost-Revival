#!/usr/bin/env bash
# Runs LCDHost with simulated Logitech HID devices (tests/hid/fake-hidapi.c,
# loaded with LD_PRELOAD) and checks what the Logitech drivers do with them:
# devices found, LCD interfaces recognized, keys reported, permission errors.
# Linux only. Usage: tests/hid/run-fake-devices.sh <path to LCDHost.app/bin> [seconds]
set -u

BIN_DIR=$(cd "$1" && pwd)
DURATION=${2:-15}
SRC_DIR=$(cd "$(dirname "$0")/../.." && pwd)
PLUGINS="Lg160x43 Lg320x240 LgBacklight"

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
CFLAGS_HIDAPI=$(pkg-config --cflags hidapi-hidraw)
gcc -shared -fPIC -O1 -Wall $CFLAGS_HIDAPI -o "$WORK/fake-hidapi.so" "$SRC_DIR/tests/hid/fake-hidapi.c" || exit 1

export HOME="$WORK/home"
export XDG_CONFIG_HOME="$HOME/.config"
export QT_QPA_PLATFORM=offscreen
mkdir -p "$HOME/Documents/LCDHost" "$HOME/.config/Link Data"
if [ ! -d "$BIN_DIR/../layouts" ]; then
    cp -r "$SRC_DIR/layouts" "$HOME/Documents/LCDHost/"
fi
{
    echo '[plugins]'
    for p in $PLUGINS; do echo "libLH_$p.so.1.0\\enabled=true"; done
} > "$HOME/.config/Link Data/LCDHost.conf"

LOG="$WORK/lcdhost.log"
(cd / && LD_PRELOAD="$WORK/fake-hidapi.so" timeout "$DURATION" "$BIN_DIR/LCDHost" > "$LOG" 2>&1)
status=$?

failed=0
expect() {
    if ! grep -qF -- "$1" "$LOG"; then
        echo "FAIL: missing \"$1\""
        failed=1
    fi
}
reject() {
    if grep -qF -- "$1" "$LOG"; then
        echo "FAIL: unexpected \"$1\""
        failed=1
    fi
}

if [ "$status" -ne 124 ]; then
    echo "FAIL: LCDHost exited early with status $status"
    failed=1
fi
# Device discovery: the G510 LCD is the second usage of its second interface,
# the G13 cannot be opened.
expect "Logitech G15 v2 ready"
expect "Logitech G510 ready"
expect "Logitech G13 found, but LCDHost has no permission to use it"
reject "LCD interface was not recognized"
# Keys, read by polling the devices.
expect "Lg160x43: Logitech G15 v2: G1 pressed"
expect "Lg160x43: Logitech G15 v2: Softbutton 0 pressed"
expect "Lg160x43: Logitech G15 v2: G1 released"
expect "Lg160x43: Logitech G510: M1 pressed"
expect "Lg160x43: Logitech G510: App pressed"
expect "Lg320x240: G19: G1 pressed"
expect "Lg320x240: G19: M1 pressed"
expect "Lg320x240: G19: Light pressed"
expect "Lg320x240: G19: Light released"
# Backlight: one entry per keyboard, not per HID interface.
expect "LgBacklight: Logitech G15 v2, Logitech G19, Logitech G510 found."
# The G15 v2 is unplugged after 7 s, while LCDHost draws on it.
expect "fake-hidapi: close fake/g15v2"
expect "Lg160x43: Logitech G510 ready"
expect "LgBacklight: Logitech G19, Logitech G510 found."

if [ "$failed" -ne 0 ]; then
    echo "--- relevant log lines ---"
    grep -E "fake-hidapi|Lg160x43|Lg320x240|LgBacklight|HID" "$LOG" | head -n 80
    exit 1
fi
echo "OK: the Logitech drivers handled the simulated devices"
