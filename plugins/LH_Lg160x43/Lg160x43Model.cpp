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

#include "Lg160x43Model.h"

// Key names: "App" is the round button left of (G15, G510, Z-10) or below
// (G13) the LCD, "Softbutton 0" to "Softbutton 3" the four buttons under it,
// as the Logitech LCD SDK plugin names them, so that layouts work with both.

// G15 (first version), report 2. The keys are spread over the bytes.
static const Lg160x43Key g15_keys[] = {
    { "G1", 1, 0x01 }, { "G2", 2, 0x02 }, { "G3", 3, 0x04 },
    { "G4", 4, 0x08 }, { "G5", 5, 0x10 }, { "G6", 6, 0x20 },
    { "G7", 2, 0x01 }, { "G8", 3, 0x02 }, { "G9", 4, 0x04 },
    { "G10", 5, 0x08 }, { "G11", 6, 0x10 }, { "G12", 7, 0x20 },
    { "G13", 1, 0x04 }, { "G14", 2, 0x08 }, { "G15", 3, 0x10 },
    { "G16", 4, 0x20 }, { "G17", 5, 0x40 }, { "G18", 8, 0x40 },
    { "M1", 6, 0x01 }, { "M2", 7, 0x02 }, { "M3", 8, 0x04 }, { "MR", 7, 0x40 },
    { "App", 8, 0x80 },
    { "Softbutton 0", 2, 0x80 }, { "Softbutton 1", 3, 0x80 },
    { "Softbutton 2", 4, 0x80 }, { "Softbutton 3", 5, 0x80 },
    { "Light", 1, 0x80 },
};

// G15 v2, report 2.
static const Lg160x43Key g15v2_keys[] = {
    { "G1", 1, 0x01 }, { "G2", 1, 0x02 }, { "G3", 1, 0x04 },
    { "G4", 1, 0x08 }, { "G5", 1, 0x10 }, { "G6", 1, 0x20 },
    { "M1", 1, 0x40 }, { "M2", 1, 0x80 }, { "M3", 2, 0x20 }, { "MR", 2, 0x40 },
    { "App", 2, 0x80 },
    { "Softbutton 0", 2, 0x02 }, { "Softbutton 1", 2, 0x04 },
    { "Softbutton 2", 2, 0x08 }, { "Softbutton 3", 2, 0x10 },
    { "Light", 2, 0x01 },
};

// G13, report 1: joystick X and Y in bytes 1 and 2, then a bit field (bit n
// in byte 3 + n / 8). Bit 23 is the backlight state, not a key.
static const Lg160x43Key g13_keys[] = {
    { "G1", 3, 0x01 }, { "G2", 3, 0x02 }, { "G3", 3, 0x04 }, { "G4", 3, 0x08 },
    { "G5", 3, 0x10 }, { "G6", 3, 0x20 }, { "G7", 3, 0x40 }, { "G8", 3, 0x80 },
    { "G9", 4, 0x01 }, { "G10", 4, 0x02 }, { "G11", 4, 0x04 }, { "G12", 4, 0x08 },
    { "G13", 4, 0x10 }, { "G14", 4, 0x20 }, { "G15", 4, 0x40 }, { "G16", 4, 0x80 },
    { "G17", 5, 0x01 }, { "G18", 5, 0x02 }, { "G19", 5, 0x04 }, { "G20", 5, 0x08 },
    { "G21", 5, 0x10 }, { "G22", 5, 0x20 },
    { "App", 6, 0x01 },
    { "Softbutton 0", 6, 0x02 }, { "Softbutton 1", 6, 0x04 },
    { "Softbutton 2", 6, 0x08 }, { "Softbutton 3", 6, 0x10 },
    { "M1", 6, 0x20 }, { "M2", 6, 0x40 }, { "M3", 6, 0x80 }, { "MR", 7, 0x01 },
    { "Stick 1", 7, 0x02 }, { "Stick 2", 7, 0x04 }, { "Stick 3", 7, 0x08 },
    { "Light", 7, 0x20 },
};

// G510, report 3. The order of the five LCD keys (App first) is assumed to
// follow the G15.
static const Lg160x43Key g510_keys[] = {
    { "G1", 1, 0x01 }, { "G2", 1, 0x02 }, { "G3", 1, 0x04 }, { "G4", 1, 0x08 },
    { "G5", 1, 0x10 }, { "G6", 1, 0x20 }, { "G7", 1, 0x40 }, { "G8", 1, 0x80 },
    { "G9", 2, 0x01 }, { "G10", 2, 0x02 }, { "G11", 2, 0x04 }, { "G12", 2, 0x08 },
    { "G13", 2, 0x10 }, { "G14", 2, 0x20 }, { "G15", 2, 0x40 }, { "G16", 2, 0x80 },
    { "G17", 3, 0x01 }, { "G18", 3, 0x02 },
    { "M1", 3, 0x10 }, { "M2", 3, 0x20 }, { "M3", 3, 0x40 }, { "MR", 3, 0x80 },
    { "App", 4, 0x01 },
    { "Softbutton 0", 4, 0x02 }, { "Softbutton 1", 4, 0x04 },
    { "Softbutton 2", 4, 0x08 }, { "Softbutton 3", 4, 0x10 },
    { "Headphone mute", 4, 0x20 }, { "Microphone mute", 4, 0x40 },
};

// Z-10 speakers, report 2: the LCD keys at the same places as on the G15.
static const Lg160x43Key z10_keys[] = {
    { "App", 8, 0x80 },
    { "Softbutton 0", 2, 0x80 }, { "Softbutton 1", 3, 0x80 },
    { "Softbutton 2", 4, 0x80 }, { "Softbutton 3", 5, 0x80 },
};

#define KEYS(table) table, int(sizeof(table) / sizeof(table[0]))

static const Lg160x43Model models[] = {
    { 0xC222, "Logitech G15", 3, 2, 9, KEYS(g15_keys) },
    { 0xC227, "Logitech G15 v2", 3, 2, 5, KEYS(g15v2_keys) },
    { 0xC21C, "Logitech G13", 3, 1, 8, KEYS(g13_keys) },
    { 0xC22D, "Logitech G510", 3, 3, 5, KEYS(g510_keys) },
    { 0xC22E, "Logitech G510", 3, 3, 5, KEYS(g510_keys) }, // with its USB audio on
    { 0x0A07, "Logitech Z-10", 3, 2, 9, KEYS(z10_keys) },
};

const Lg160x43Model *Lg160x43Model::find(int vendor_id, int product_id)
{
    if(vendor_id != 0x046D)
        return 0;
    for(unsigned i = 0; i < sizeof(models) / sizeof(models[0]); ++i)
        if(models[i].product_id == product_id)
            return &models[i];
    return 0;
}

const char *Lg160x43Model::supportedText()
{
    return "G13, G15, G510 or Z-10";
}

bool Lg160x43Model::decodeKeys(const QByteArray &report, quint64 *pressed) const
{
    if(!key_report_id || report.size() < key_report_size ||
            (unsigned char) report.at(0) != key_report_id)
        return false;
    quint64 bits = 0;
    for(int i = 0; i < key_count && i < 64; ++i)
        if((unsigned char) report.at(keys[i].byte) & keys[i].mask)
            bits |= quint64(1) << i;
    *pressed = bits;
    return true;
}
