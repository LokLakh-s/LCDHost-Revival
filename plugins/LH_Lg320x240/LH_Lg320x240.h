/**
  \file     LH_Lg320x240.h
  @author   Johan Lindh <johan@linkdata.se>
  Copyright (c) 2009-2011, Johan Lindh

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

#ifndef LH_LG320x240_H
#define LH_LG320x240_H

#include <QEvent>
#include <QString>
#include <LH_QtPlugin.h>

typedef struct libusb_context libusb_context;
class LH_Qt_QString;
class QWinEventNotifier;

class LH_Lg320x240 : public LH_QtPlugin
{
    Q_OBJECT

    int timer_id_;
    libusb_context *usb_ctx_;
    LH_Qt_QString *setup_status_;
    LH_Qt_QString *setup_install_driver_;
    LH_Qt_QString *setup_remove_driver_;

    // Windows: the elevated LCDHostDriverSetup.exe process, while it runs.
    void *driver_setup_process_;
    QWinEventNotifier *driver_setup_notifier_;
    QString driver_setup_command_;
    QString driver_setup_log_;

    void closeDevice();
    void runDriverSetup(const QString &command);
    void stopDriverSetup();

public:
    LH_Lg320x240();
    const char *userInit();
    void userTerm();
    void timerEvent(QTimerEvent *ev);

public slots:
    void setStatus(const QString &text);
    void installDriver();
    void removeDriver();
    void driverSetupFinished();
};

#endif // LH_LG320x240_H
