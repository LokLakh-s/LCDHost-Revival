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

#include "G19Keys.h"

namespace
{
struct Key
{
    const char *name;
    int byte;
    unsigned char mask;
};

// From the original LCDHost raw input code, confirmed by pylibg19 (Gnome15).
const Key keys[] = {
    { "G1", 1, 0x01 }, { "G2", 1, 0x02 }, { "G3", 1, 0x04 }, { "G4", 1, 0x08 },
    { "G5", 1, 0x10 }, { "G6", 1, 0x20 }, { "G7", 1, 0x40 }, { "G8", 1, 0x80 },
    { "G9", 2, 0x01 }, { "G10", 2, 0x02 }, { "G11", 2, 0x04 }, { "G12", 2, 0x08 },
    { "M1", 2, 0x10 }, { "M2", 2, 0x20 }, { "M3", 2, 0x40 }, { "MR", 2, 0x80 },
    { "Light", 3, 0x08 },
};

const int key_count = int(sizeof(keys) / sizeof(keys[0]));
}

int G19Keys::count()
{
    return key_count;
}

const char *G19Keys::name(int index)
{
    return (index >= 0 && index < key_count) ? keys[index].name : "Unknown";
}

bool G19Keys::decode(const QByteArray &report, quint32 *pressed)
{
    if(report.size() < ReportSize || (unsigned char) report.at(0) != ReportId)
        return false;
    quint32 bits = 0;
    for(int i = 0; i < key_count; ++i)
        if((unsigned char) report.at(keys[i].byte) & keys[i].mask)
            bits |= quint32(1) << i;
    *pressed = bits;
    return true;
}
