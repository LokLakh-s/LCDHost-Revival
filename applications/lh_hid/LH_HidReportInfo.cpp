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

#include "LH_HidReportInfo.h"

#include <QList>

LH_HidReportInfo::LH_HidReportInfo() :
    valid_(false),
    uses_ids_(false)
{
}

// Walks the items of the descriptor (HID 1.11, section 6.2.2) and adds up the
// bits of the Input, Output and Feature main items per report ID.
LH_HidReportInfo::LH_HidReportInfo(const QByteArray &descriptor) :
    valid_(false),
    uses_ids_(false)
{
    struct Globals
    {
        qint64 report_size;
        qint64 report_count;
        int report_id;
    };

    const unsigned char *data = (const unsigned char *) descriptor.constData();
    const int length = descriptor.size();
    Globals globals = { 0, 0, 0 };
    QList<Globals> stack;
    int pos = 0;

    if(length == 0)
        return;

    while(pos < length)
    {
        const unsigned char prefix = data[pos++];

        if(prefix == 0xFE)
        {
            // Long item: data size, tag, data. None of them matters here.
            if(pos + 2 > length)
                return;
            pos += 2 + data[pos];
            continue;
        }

        int size = prefix & 0x03;
        if(size == 3)
            size = 4;
        if(pos + size > length)
            return;
        quint32 value = 0;
        for(int i = 0; i < size; ++i)
            value |= quint32(data[pos + i]) << (8 * i);
        pos += size;

        Type type;
        switch(prefix & 0xFC)
        {
        case 0x74: // Report Size
            globals.report_size = value;
            continue;
        case 0x94: // Report Count
            globals.report_count = value;
            continue;
        case 0x84: // Report ID
            globals.report_id = int(value);
            uses_ids_ = true;
            continue;
        case 0xA4: // Push
            stack.append(globals);
            continue;
        case 0xB4: // Pop
            if(!stack.isEmpty())
                globals = stack.takeLast();
            continue;
        case 0x80:
            type = Input;
            break;
        case 0x90:
            type = Output;
            break;
        case 0xB0:
            type = Feature;
            break;
        default:
            continue;
        }
        bits_[type][globals.report_id] += globals.report_size * globals.report_count;
    }

    valid_ = (pos == length);
}

int LH_HidReportInfo::size(Type type, int report_id) const
{
    if(type < Input || type > Feature)
        return 0;
    QMap<int, qint64>::const_iterator it = bits_[type].constFind(report_id);
    if(it == bits_[type].constEnd() || it.value() <= 0)
        return 0;
    return int((it.value() + 7) / 8) + (uses_ids_ ? 1 : 0);
}
