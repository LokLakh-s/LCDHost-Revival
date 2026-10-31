# Hardware setup

LCDHost drives Logitech keyboard and speaker LCDs **directly**, without
Logitech Gaming Software (LGS) or G HUB. This page explains what each
operating system needs so that LCDHost can reach the device.

> The maintainers do not own these devices yet, so the steps below are based
> on the code, on upstream documentation and on other open-source projects.
> Please report what works for you with the *Hardware test report* issue form.

## Supported devices

| Device | USB ID | LCD | LCDHost driver plugin |
|---|---|---|---|
| G19 / G19s | 046d:c229 | 320×240 colour | `Lg320x240` (libusb) |
| G15 v1 | 046d:c222 | 160×43 mono | `Lg160x43` (HID) |
| G15 v2 | 046d:c227 | 160×43 mono | `Lg160x43` (HID) |
| G13 | 046d:c21c | 160×43 mono | `Lg160x43` (HID) |
| G510 | 046d:c22d, 046d:c22e | 160×43 mono | `Lg160x43` (HID) |
| Z-10 speakers | 046d:0a07 | 160×43 mono | `Lg160x43` (HID) |

Backlight control (`LgBacklight` plugin) uses HID on the G11, G13, G15 v2,
G510 and G19. The `LgLcdMan` plugin is an alternative that goes through the
proprietary Logitech LCD SDK and therefore needs LGS; it is not needed when
the direct drivers above are used.

The G19 is a composite device: interface 0 (vendor specific) carries the
display (bulk endpoint 0x02) and the menu keys (interrupt endpoint 0x81);
interface 1 (HID) carries the G-keys, M-keys and backlight. LCDHost only
claims interface 0.

## Linux

The 160×43 devices work through `hidraw`, which stays available even when the
kernel driver `hid-lg-g15` is loaded (it also handles the G-keys and LEDs), so
nothing needs to be detached. No kernel driver binds the G19 display
interface, so libusb can claim it directly.

Only permissions are needed (for the G19, the settings of the *Lg320x240*
plugin in the **Plugins** tab tell when they are missing). Install the udev
rules shipped in
[`packaging/linux/70-lcdhost.rules`](../packaging/linux/70-lcdhost.rules):

```sh
sudo cp packaging/linux/70-lcdhost.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

The rules grant access to the user of the local session (`TAG+="uaccess"`).
For access over SSH, see the comment at the top of the file.

## Windows

### 160×43 devices (G15, G13, G510, Z-10)

These are standard HID devices: no driver installation is needed. Quit LGS
or G HUB first, since they use the LCD too.

### G19 / G19s

libusb needs the **display interface** of the G19 (`USB\VID_046D&PID_C229&MI_00`)
to use Microsoft's generic **WinUSB** driver. Only that interface changes: the
keyboard (046d:c228) and the G-keys interface (`MI_01`) keep their standard
drivers.

Until LCDHost installs the driver by itself (see the [roadmap](../ROADMAP.md)),
do it once with [Zadig](https://zadig.akeo.ie/):

1. Quit LGS if it is installed; while WinUSB is bound, LGS cannot use the
   display.
2. In Zadig, enable *Options → List All Devices* and select the entry whose
   USB ID is **`046D C229 00`** (interface 0 of the G19 display device; its
   name mentions the G19 and "Interface 0" or "Display interface").
3. Select **WinUSB** as target driver and click *Replace Driver* (or
   *Install Driver*).

Zadig creates a driver package signed with a certificate generated on your
machine; the driver that actually runs, `winusb.sys`, is part of Windows.

**To undo it:** in Device Manager, find the G19 display interface under
*Universal Serial Bus devices*, uninstall it with *Delete the driver software
for this device* ticked, then unplug and replug the keyboard. Windows should
then use the Logitech driver again if LGS is installed, or leave the
interface without a driver.

## Prior art

Other open-source projects driving these devices, useful for reference:
[lcdproc](https://github.com/lcdproc/lcdproc) (G15/G510/Z-10 through hidraw),
[g19daemon](https://github.com/mortendynamite/g19daemon) (G19 through libusb),
[Gnome15](https://github.com/CMoH/gnome15) (archived).
