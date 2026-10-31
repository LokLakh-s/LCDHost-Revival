# Architecture overview

This document gives new contributors a map of the code base. It describes the
current (inherited) design; planned changes are listed in the
[roadmap](../ROADMAP.md).

## Big picture

```
            +-------------------------- LCDHost (host application) ---------------------------+
            |                                                                                  |
 layout --> |  layout tree (instances)  -->  composition (QGraphicsScene)  -->  output device  | --> LCD / virtual LCD
  (XML)     |        ^                                                             ^           |
            |        | classes                                                     | drivers   |
            +--------|-------------------------------------------------------------|-----------+
                     |                                                             |
             layout plugins (LH_Text, LH_Dial, ...)              device plugins (LH_Lg320x240, LH_VirtualLCD, ...)
```

- The **host** (`applications/LCDHost`) loads plugins, keeps the layout tree,
  composites it and sends the resulting image to the selected output device.
- **Plugins** are shared libraries. A plugin can provide **layout classes**
  (element types such as "Text" or "Dial"), **output devices** (drivers), or
  both.
- A **layout** is an XML file describing a tree of **instances**: each
  instance is an object of a layout class, with a position, a size and its
  settings.

## Repository layout

| Path | Contents |
|---|---|
| `applications/LCDHost/` | The host application: main window, layout editor, plugin loading, rendering, device handling. |
| `applications/lh_hid/` | Shared HID access library (wraps the system's hidapi: device discovery, report descriptors, input reports), used by the Logitech drivers, and the Linux udev rules installer (`LH_UdevRules`). |
| `applications/LCDHostDriverSetup/` | Windows only: elevated helper that installs or removes the WinUSB driver of the G19 display, built on libwdi. |
| `applications/lh_logger/` | Logging to `Documents/LCDHost/logs/`. |
| `linkdata/lh_api5plugin/` | The plugin API: `lh_plugin.h` (C ABI) and the `LH_Qt*` C++ helper classes most plugins are built on. |
| `codeleap/` | Libraries shared by the CodeLeap plugins: JSON (`json`), conditional formatting (`ConditionalFormatting`), offline translation cache (`SimpleTranslator`). |
| `plugins/` | One directory per plugin; `LgCommon/` holds code shared by the Logitech drivers. |
| `3rdParty/` | Bundled third-party libraries (TagLib, zlib), see [THIRD_PARTY.md](THIRD_PARTY.md). |
| `lh_features/`, `qmakecache/`, `.qmake.cache.in` | qmake helpers: shared build settings and optional features (shared libraries a project can pull in) defined in `lh_features/lh_*.pri`. |
| `layouts/` | Default layouts installed on first run. |
| `packaging/` | Linux udev rules, desktop file and AppImage script (`linux/build-appimage.sh`), the Windows dependency script (`windows/fetch-deps.ps1`) and the release packaging used by the CI. |
| `tests/` | Smoke test, HID unit tests and simulated Logitech devices (`hid/`), driver installer test and test layouts for individual plugins. |

## The plugin API

The API is defined in `linkdata/lh_api5plugin/lh_plugin.h` (API version 5).
It is a plain C interface so that plugins do not depend on the exact compiler
or Qt build of the host, although in practice all bundled plugins use Qt.

### Discovery and loading

1. Every plugin embeds an XML document (`<lcdhostplugin>…</lcdhostplugin>`)
   with its id, API version, author and description. The host finds it by
   scanning the file, **without loading the library**, so the Plugins tab can
   list plugins that are not loaded.
2. When the user enables a plugin, the host loads the library in a dedicated
   thread (`AppLibraryThread`). Each plugin runs in its own thread; a crash
   (segmentation fault, illegal instruction…) is caught with a signal handler
   and `longjmp`, and the plugin is marked as failed instead of taking the
   host down.
3. The host calls `lh_create()`, gets the object call table and calls
   `obj_init()`. The plugin then reports its classes through
   `obj_class_list()` and its devices through the `lh_cb_arrive` callback.

### Objects, callbacks and notifications

- Plugins, instances and devices are all *objects* described by call tables
  (`lh_object_calltable`, `lh_instance_calltable`, `lh_device_calltable`).
- Plugins talk to the host through a single callback (`lh_callback_t`) with a
  code (`lh_cb_render`, `lh_cb_setup_refresh`, `lh_cb_arrive`, …).
- The host pushes system data (CPU, memory, network) through
  `lh_systemstate` and `obj_notify()`, filtered by `LH_NOTE_*` flags.

### Setup items

Settings shown in the UI and saved in layouts are *setup items*
(`lh_setup_item`): typed values (integer, colour, font, file, string list…)
with flags and optional links. A link lets one item follow another item's
value, which is how data flows between instances (for example a dial showing
a value published by a monitoring instance). In C++, the `LH_Qt_*` classes
(`LH_Qt_int`, `LH_Qt_QColor`, …) wrap setup items.

### Rendering

- An instance renders itself into a `QImage` (`obj_render_qimage`) or a blob
  (`obj_render_blob`) of the size the host asks for.
- The host composites instances with a `QGraphicsScene`, applying layout
  positions and alignment rules, then passes the frame to the device's
  `obj_render_qimage`, `obj_render_argb32` or `obj_render_mono`.
- Devices report buttons and other inputs through `lh_cb_input`; layouts can
  react to them (see the Cursor and Logic plugins).

## Writing a plugin

The easiest starting point is an existing small plugin:

- `plugins/LH_Decor`: minimal layout classes built on `LH_QtInstance`.
- `plugins/LH_VirtualLCD`: a minimal output device built on `LH_QtDevice`.

A plugin is a qmake `lib` project that includes `plugins/Plugins.pri`,
defines `__lcdhostplugin_xml` (see `lh_plugin.h`) and uses `LH_PLUGIN()` and
`LH_PLUGIN_CLASS()` from the Qt helper classes.

## Layout files

Layouts are XML files saved next to their resources (images, fonts) in
`Documents/LCDHost/layouts/<name>/`. They store the instance tree, each
instance's class id, geometry and setup item values. Keep backward
compatibility in mind: layouts made with old versions of LCDHost are still in
use.
