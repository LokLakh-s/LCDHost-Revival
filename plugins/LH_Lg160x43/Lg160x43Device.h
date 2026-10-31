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

#ifndef LG160X43DEVICE_H
#define LG160X43DEVICE_H

#include "LH_QtDevice.h"

class LH_HidDevice;
struct Lg160x43Model;

// A Logitech 160x43 LCD. On Windows, the LCD and the keys can be in
// different top-level collections of the HID interface, so they can come
// from two LH_HidDevice objects.
class Lg160x43Device : public LH_QtDevice
{
    Q_OBJECT

    const Lg160x43Model *model_;
    LH_HidDevice *lcd_;
    LH_HidDevice *keys_;
    quint64 pressed_;
    bool opened_;

public:
    Lg160x43Device(const Lg160x43Model *model, LH_HidDevice *lcd,
                   const QString &devid, LH_QtPlugin *drv);
    ~Lg160x43Device();

    const Lg160x43Model *model() const { return model_; }
    LH_HidDevice *lcd() const { return lcd_; }
    LH_HidDevice *keys() const { return keys_; }
    // Starts reporting the keys of hd (0: stop).
    void setKeys(LH_HidDevice *hd);
    // True while LCDHost renders to the device.
    bool isOpen() const { return opened_; }

    const char* open();
    const char* render_qimage(QImage *img);
    const char* close();

signals:
    void openChanged();

public slots:
    void inputReport(const QByteArray &report);

private:
    void sendKey(int index, bool down);
};

#endif // LG160X43DEVICE_H
