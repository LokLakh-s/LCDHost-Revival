/**
  \file     LH_Lg320x240.cpp
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

#include <QtGlobal>
#include <QDebug>
#include <QTimerEvent>

#include <libusb.h>

#include "LH_QtDevice.h"
#include "LH_Qt_QString.h"
#include "LH_Lg320x240.h"
#include "LogitechG19.h"

LH_PLUGIN(LH_Lg320x240)

char __lcdhostplugin_xml[] =
"<?xml version=\"1.0\"?>"
"<lcdhostplugin>"
  "<id>Lg320x240</id>"
  "<rev>" STRINGIZE(REVISION) "</rev>"
  "<api>" STRINGIZE(LH_API_MAJOR) "." STRINGIZE(LH_API_MINOR) "</api>"
  "<ver>" "r" STRINGIZE(REVISION) "</ver>"
  "<author>Johan \"SirReal\" Lindh</author>"
  "<homepageurl><a href=\"http://www.linkdata.se/software/lcdhost\">Link Data Stockholm</a></homepageurl>"
  "<logourl></logourl>"
  "<shortdesc>"
  "Logitech 320x240 LCD via USB"
  "</shortdesc>"
  "<longdesc>"
    "USB-level driver for Logitech 320x240 LCD displays, such as the G19.<br/>"
    "On Windows, the display interface of the G19 (USB ID 046D C229, interface 0) must use the "
    "<a href=\"https://learn.microsoft.com/windows-hardware/drivers/usbcon/winusb\">WinUSB</a> driver; "
    "<a href=\"https://zadig.akeo.ie/\">Zadig</a> can install it. On Linux, install the LCDHost udev rules. "
    "See <a href=\"https://github.com/LokLakh-s/LCDHost-Revival/blob/master/docs/HARDWARE.md\">docs/HARDWARE.md</a>."
  "</longdesc>"
"</lcdhostplugin>";

static const char hardware_guide_url[] =
        "https://github.com/LokLakh-s/LCDHost-Revival/blob/master/docs/HARDWARE.md";

LH_Lg320x240::LH_Lg320x240() :
    LH_QtPlugin(),
    timer_id_(0),
    usb_ctx_(0),
    setup_status_(0)
{
}

const char *LH_Lg320x240::userInit()
{
    const int info_flags = LH_FLAG_NOSAVE_DATA | LH_FLAG_NOSAVE_LINK | LH_FLAG_NOSOURCE | LH_FLAG_NOSINK;

    setup_status_ = new LH_Qt_QString(this, "Status", "Looking for a G19...",
                                      info_flags, lh_type_string_html);
    new LH_Qt_QString(this, "~guide",
                      QString("<a href=\"%1\">Hardware setup guide</a>").arg(hardware_guide_url),
                      info_flags | LH_FLAG_HIDETITLE, lh_type_string_html);

    if(!usb_ctx_)
    {
        int retv = libusb_init(&usb_ctx_);
        if(retv != LIBUSB_SUCCESS)
        {
            usb_ctx_ = 0;
            setStatus(QString("USB access is not available (libusb: %1).")
                      .arg(QString::fromLatin1(libusb_error_name(retv))));
            return NULL;
        }
        libusb_set_option(usb_ctx_, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_ERROR);
    }

    if(!timer_id_)
        timer_id_ = startTimer(2000);

    return NULL;
}

void LH_Lg320x240::userTerm()
{
    if(timer_id_)
    {
        killTimer(timer_id_);
        timer_id_ = 0;
    }
    closeDevice();
    if(usb_ctx_)
    {
        libusb_exit(usb_ctx_);
        usb_ctx_ = 0;
    }
}

void LH_Lg320x240::setStatus(const QString &text)
{
    if(setup_status_ && setup_status_->value() != text)
    {
        qDebug("Lg320x240: %s", qPrintable(text));
        setup_status_->setValue(text);
    }
}

void LH_Lg320x240::closeDevice()
{
    if(LogitechG19 *g19 = findChild<LogitechG19 *>())
    {
        g19->userTerm();
        delete g19;
    }
}

void LH_Lg320x240::timerEvent(QTimerEvent *ev)
{
    if(!usb_ctx_ || ev->timerId() != timer_id_)
        return;
    // A present device reports its own state.
    if(findChild<LogitechG19 *>())
        return;

    libusb_device **list = 0;
    libusb_device *usbdev = 0;
    struct libusb_device_descriptor dd;
    ssize_t count = libusb_get_device_list(usb_ctx_, &list);
    for(ssize_t i = 0; i < count && !usbdev; ++i)
    {
        if(!libusb_get_device_descriptor(list[i], &dd) &&
                dd.idVendor == 0x046d && dd.idProduct == 0xc229)
            usbdev = libusb_ref_device(list[i]);
    }
    if(list)
        libusb_free_device_list(list, 1);

    if(!usbdev)
    {
        setStatus("No G19 connected.");
        return;
    }

    // Only offer the display when LCDHost can actually use it, and say why not.
    int retv = LogitechG19::probe(usbdev);
    if(retv != LIBUSB_SUCCESS)
    {
        setStatus(LogitechG19::errorText(retv));
        libusb_unref_device(usbdev);
        return;
    }

    LogitechG19 *g19 = new LogitechG19(usb_ctx_, usbdev, &dd, this);
    connect(g19, SIGNAL(statusChanged(QString)), this, SLOT(setStatus(QString)));
    setStatus(LogitechG19::readyText());
}
