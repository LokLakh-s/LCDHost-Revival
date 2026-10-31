/*
  Copyright (c) 2026 The LCDHost Revival contributors

  This file is part of LCDHost.

  LCDHost is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LCDHost is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LCDHost.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LG160X43MODEL_H
#define LG160X43MODEL_H

#include <QByteArray>
#include <QtGlobal>

// A key of a Logitech 160x43 device: pressed while (report[byte] & mask).
struct Lg160x43Key
{
    const char *name; // control name reported to layouts
    int byte;         // index in the input report, report ID at index 0
    unsigned char mask;
};

// The Logitech devices with a 160x43 monochrome LCD driven over HID. The
// report layouts come from the Linux kernel driver hid-lg-g15 and from the
// original LCDHost raw input code; the G510 and Z-10 ones are not confirmed
// on real devices yet.
struct Lg160x43Model
{
    int product_id;
    const char *name;
    int lcd_report_id;   // output report that carries a frame
    int key_report_id;   // input report that carries the keys (0: none)
    int key_report_size; // its size in bytes, report ID included
    const Lg160x43Key *keys;
    int key_count;

    // NULL if the product is not a 160x43 device.
    static const Lg160x43Model *find(int vendor_id, int product_id);
    // Text listing the supported devices, for status messages.
    static const char *supportedText();

    // Bit i of the result is set while keys[i] is pressed. Returns false if
    // the report is not a key report of this model.
    bool decodeKeys(const QByteArray &report, quint64 *pressed) const;
};

#endif // LG160X43MODEL_H
