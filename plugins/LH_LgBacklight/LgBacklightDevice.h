/*
  Copyright (c) 2009-2016 Johan Lindh <johan@linkdata.se>
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

#ifndef LGBACKLIGHTDEVICE_H
#define LGBACKLIGHTDEVICE_H

#include <QObject>
#include <QString>
#include <QColor>

class LH_HidDevice;

// The Logitech keyboards with a backlight that HID feature reports control.
// The reports come from the Linux kernel driver hid-lg-g15 (G13, G15, G510)
// and from Gnome15 (G19).
struct LgBacklightModel
{
    enum Kind
    {
        Rgb,   // [id, red, green, blue(, 0)]
        Levels // [id, 1, level 0-2, 0]: off, dim, bright
    };

    int product_id;
    const char *name;
    Kind kind;
    int report_id;
    int report_size; // used when the report descriptor is not available

    // NULL if the device has no backlight that LCDHost can control.
    static const LgBacklightModel *find(int vendor_id, int product_id);
    static const char *supportedText();
};

class LgBacklightDevice : public QObject
{
    Q_OBJECT

    LH_HidDevice *hd_;
    const LgBacklightModel *model_;
    int report_size_;
    QColor color_;
    int red_max_;
    int blue_max_;
    bool white_balance_;

public:
    LgBacklightDevice(LH_HidDevice *hd, const LgBacklightModel *model,
                      int report_size, QObject *parent);

    LH_HidDevice *hd() const { return hd_; }
    const LgBacklightModel *model() const { return model_; }
    QColor color() const { return color_; }
    void setColor(QColor c);

    QColor getDeviceColor();
    bool setDeviceColor(const QColor &c);
    bool useWhiteBalance() const { return white_balance_; }
    void setUseWhiteBalance(bool b) { white_balance_ = b; }

signals:
    void colorChanged();
};

#endif // LGBACKLIGHTDEVICE_H
