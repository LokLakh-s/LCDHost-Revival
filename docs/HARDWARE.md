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

The driver plugins also report the keys to layouts (see [Keys](#keys)).
Backlight control (`LgBacklight` plugin) uses HID: a colour on the G13, G19
and G510, a level (off, dim, bright) on the G11, G15 and G15 v2.

The `LgLcdMan` plugin, an alternative that goes through Logitech Gaming
Software and the proprietary Logitech LCD SDK, is not part of the releases
since 0.3.0 (developers can build it with their own copy of the SDK, see the
README). The drivers above do not need any Logitech software.

The G19 is a composite device: interface 0 (vendor specific) carries the
display (bulk endpoint 0x02) and the LCD keys (interrupt endpoint 0x81);
interface 1 (HID) carries the G-keys, M-keys and backlight. LCDHost claims
interface 0 through libusb and uses interface 1 through the operating
system's HID driver, which it keeps.

## Linux

The 160×43 devices work through `hidraw`, which stays available even when the
kernel driver `hid-lg-g15` is loaded (it also handles the G-keys and LEDs), so
nothing needs to be detached. No kernel driver binds the G19 display
interface, so libusb can claim it directly.

Only permissions are needed: the LCDHost udev rules
([`packaging/linux/70-lcdhost.rules`](../packaging/linux/70-lcdhost.rules))
give the user of the local session access to the devices
(`TAG+="uaccess"`). The settings of the Logitech plugins (*Lg160x43*,
*Lg320x240*, *LgBacklight*) in the **Plugins** tab tell when access is
missing.

**From LCDHost** (0.3.0 and later): click **Install udev rules** in the
settings of one of these plugins and give the administrator password.
LCDHost runs, as root through `pkexec`, a short script that writes the rules
built into LCDHost to `/etc/udev/rules.d/70-lcdhost.rules`, reloads the udev
rules and applies them to the connected devices; within a few seconds the
status reads "ready". **Remove udev rules** deletes that file again. This
works the same from the AppImage.

**From a terminal**, in the source tree:

```sh
sudo cp packaging/linux/70-lcdhost.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

For access over SSH or without a local session, see the comment at the top
of the rules file.

## Windows

### 160×43 devices (G15, G13, G510, Z-10)

These are standard HID devices: no driver installation is needed. Quit LGS
first, since it draws on the LCD too; the settings of the *Lg160x43* plugin
tell when it is running.

### G19 / G19s

libusb needs the **display interface** of the G19 (`USB\VID_046D&PID_C229&MI_00`)
to use Microsoft's generic **WinUSB** driver. Only that interface changes: the
keyboard (046d:c228) and the G-keys interface (`MI_01`) keep their standard
drivers. While WinUSB is installed, LGS cannot use the display.

**From LCDHost** (0.2.0 and later):

1. In the **Plugins** tab, load the *Lg320x240* plugin and select it. Its
   settings show whether LCDHost can use the G19 and, if not, why.
2. Click **Install driver** and accept the administrator prompt. The G19
   does not need to be plugged in: the driver then waits in the Windows
   driver store until it is.
3. Within a few seconds the status reads "G19 ready". Select the G19 in the
   **Output** tab.

`LCDHostDriverSetup.exe`, next to `LCDHost.exe`, does the work with
[libwdi](https://github.com/pbatard/libwdi), the library behind Zadig. It
generates a driver package for the display interface and signs it with a
certificate created on your computer: the certificate is added to the *Trusted
Root Certification Authorities* and *Trusted Publishers* stores of the local
machine, and its private key is deleted right after signing, so it cannot sign
anything else. The driver that actually runs, `winusb.sys`, is part of
Windows. From an administrator command prompt, `LCDHostDriverSetup install`
and `LCDHostDriverSetup uninstall` do the same as the buttons.

**To undo it:** click **Remove driver**. It removes the driver package and
the certificate, and Windows gives the display interface its next best driver:
Logitech's if LGS is installed, none otherwise. It also removes the WinUSB
packages that Zadig installed for this interface.

**With Zadig** (builds without `LCDHostDriverSetup.exe`):

1. Quit LGS if it is installed.
2. In [Zadig](https://zadig.akeo.ie/), enable *Options → List All Devices*
   and select the entry whose USB ID is **`046D C229 00`** (interface 0 of the
   G19 display device; its name mentions the G19 and "Interface 0" or
   "Display interface").
3. Select **WinUSB** as target driver and click *Replace Driver* (or
   *Install Driver*).

To undo a Zadig installation, use **Remove driver** in LCDHost, or in Device
Manager find the G19 display interface under *Universal Serial Bus devices*,
uninstall it with *Delete the driver software for this device* ticked, then
unplug and replug the keyboard.

## Keys

The driver plugins report the keys of the devices to layouts, under these
names (the input settings of layout objects, such as the *Cursor* plugin
ones, list them when you press a key):

| Device | Keys |
|---|---|
| G19 | `G1`–`G12`, `M1`–`M3`, `MR`, `Light`; LCD keys `App`, `Cancel`, `Menu`, `Ok`, `Left`, `Right`, `Up`, `Down` (while LCDHost draws on the G19) |
| G15 | `G1`–`G18`, `M1`–`M3`, `MR`, `Light`, `App`, `Softbutton 0`–`Softbutton 3` |
| G15 v2 | `G1`–`G6`, `M1`–`M3`, `MR`, `Light`, `App`, `Softbutton 0`–`Softbutton 3` |
| G13 | `G1`–`G22`, `M1`–`M3`, `MR`, `Light`, `App`, `Softbutton 0`–`Softbutton 3`, `Stick 1`–`Stick 3` (joystick buttons) |
| G510 | `G1`–`G18`, `M1`–`M3`, `MR`, `App`, `Softbutton 0`–`Softbutton 3`, `Headphone mute`, `Microphone mute` |
| Z-10 | `App`, `Softbutton 0`–`Softbutton 3` |

`App` is the round button next to the LCD, `Softbutton 0` to `Softbutton 3`
the four buttons under it, named as by the `LgLcdMan` plugin so that layouts
work with both. Each key press and release is also written to the **Log**
tab: please include them in hardware test reports. The key layouts come from
the Linux kernel driver `hid-lg-g15`, the original LCDHost code and Gnome15;
the order of the G510 LCD keys is not confirmed yet.

Without Logitech software, the G15 and G510 also send F1–F12 (and more) for
their G-keys, as a separate keyboard; on Linux, the kernel driver
`hid-lg-g15` turns that off.

## Prior art

Other open-source projects driving these devices, useful for reference:
the Linux kernel driver
[`hid-lg-g15`](https://github.com/torvalds/linux/blob/master/drivers/hid/hid-lg-g15.c)
(G-keys, LEDs and backlight of the G13, G15, G510 and Z-10),
[lcdproc](https://github.com/lcdproc/lcdproc) (G15/G510/Z-10 through hidraw),
[g19daemon](https://github.com/mortendynamite/g19daemon) (G19 through libusb),
[Gnome15](https://github.com/CMoH/gnome15) (archived).
