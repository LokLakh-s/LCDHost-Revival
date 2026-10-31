/**
  \file     LH_Lg160x43.h
  @author   Johan Lindh <johan@linkdata.se>
  Copyright (c) 2009-2011, Johan Lindh
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

#ifndef LH_LG160X43_H
#define LH_LG160X43_H

#include <QHash>
#include <QList>
#include <QString>

#include "LH_QtPlugin.h"

class LH_HidDevice;
class LH_Qt_QString;
class Lg160x43Device;
struct Lg160x43Model;
class QTimerEvent;

class LH_Lg160x43 : public LH_QtPlugin
{
    Q_OBJECT

    enum Role { RoleLcd = 1, RoleKeys = 2 };

    struct OpenError
    {
        int code;
        QString text;
    };

    LH_Qt_QString *setup_status_;
    int timer_id_;
    // The HID devices of supported models, with what each one carries.
    QList<LH_HidDevice *> known_;
    QHash<LH_HidDevice *, int> roles_;
    QHash<LH_HidDevice *, Lg160x43Device *> devices_; // by LCD collection
    QHash<LH_HidDevice *, OpenError> errors_;
    // Products found without a recognized LCD interface, as of the last
    // timer tick: by then, all the interfaces of a device have been seen.
    QList<int> unrecognized_;

public:
    LH_Lg160x43();
    const char *userInit();
    void userTerm();

protected:
    void timerEvent(QTimerEvent *ev);

public slots:
    void onlineChanged(LH_HidDevice *hd, bool b);
    void refreshStatus();

private:
    int roles(LH_HidDevice *hd, const Lg160x43Model *model);
    void addDevice(LH_HidDevice *hd, const Lg160x43Model *model);
    void removeDevice(LH_HidDevice *hd);
    void attachKeys();
    QString uniqueDevid(LH_HidDevice *hd, const Lg160x43Model *model) const;
};

#endif // LH_LG160X43_H
