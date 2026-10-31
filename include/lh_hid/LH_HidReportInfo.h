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

#ifndef LH_HIDREPORTINFO_H
#define LH_HIDREPORTINFO_H

#include <QByteArray>
#include <QMap>

#if defined(LH_HID_LIBRARY)
# define LH_HID_REPORTINFO_EXPORT Q_DECL_EXPORT
#else
# define LH_HID_REPORTINFO_EXPORT Q_DECL_IMPORT
#endif

// The reports that a HID report descriptor declares: their IDs and sizes.
// Drivers use it to find, among the HID interfaces (Linux) or top-level
// collections (Windows) of a device, the one that carries a given report.
class LH_HID_REPORTINFO_EXPORT LH_HidReportInfo
{
public:
    enum Type { Input = 0, Output = 1, Feature = 2 };

    LH_HidReportInfo();
    explicit LH_HidReportInfo(const QByteArray &descriptor);

    // False when the descriptor is empty or truncated.
    bool isValid() const { return valid_; }
    // True when the reports start with a report ID byte.
    bool usesReportIds() const { return uses_ids_; }
    bool has(Type type, int report_id) const { return size(type, report_id) > 0; }
    // Size in bytes of a report, report ID byte included; 0 if not declared.
    int size(Type type, int report_id) const;

private:
    bool valid_;
    bool uses_ids_;
    QMap<int, qint64> bits_[3];
};

#endif // LH_HIDREPORTINFO_H
