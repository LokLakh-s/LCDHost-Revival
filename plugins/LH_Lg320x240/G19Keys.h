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

#ifndef G19KEYS_H
#define G19KEYS_H

#include <QByteArray>
#include <QtGlobal>

// The G-keys, M-keys and light key of the G19, sent by its HID interface
// (MI_01) as input report 2: [0x02, G1-G8, G9-G12 M1-M3 MR, light key].
namespace G19Keys
{
    enum { ReportId = 2, ReportSize = 4 };

    int count();
    // Control name reported to layouts.
    const char *name(int index);
    // Bit i of the result is set while key i is pressed. Returns false if
    // the report is not a key report.
    bool decode(const QByteArray &report, quint32 *pressed);
}

#endif // G19KEYS_H
