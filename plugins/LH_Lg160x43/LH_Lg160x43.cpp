/**
  \file     LH_Lg160x43.cpp
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

#include <QtGlobal>
#include <QSet>
#include <QStringList>
#include <QTimerEvent>

#include <algorithm>
#include <errno.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "LH_Lg160x43.h"
#include "Lg160x43Device.h"
#include "Lg160x43Model.h"
#include "LH_HidDevice.h"
#include "LH_HidReportInfo.h"
#include "LH_Qt_QString.h"
#include "LgUdevSetup.h"

LH_PLUGIN(LH_Lg160x43)

char __lcdhostplugin_xml[] =
"<?xml version=\"1.0\"?>"
"<lcdhostplugin>"
  "<id>Lg160x43</id>"
  "<rev>" STRINGIZE(REVISION) "</rev>"
  "<api>" STRINGIZE(LH_API_MAJOR) "." STRINGIZE(LH_API_MINOR) "</api>"
  "<ver>" "r" STRINGIZE(REVISION) "</ver>"
  "<author>Johan \"SirReal\" Lindh</author>"
  "<homepageurl><a href=\"http://www.linkdata.se/software/lcdhost\">Link Data Stockholm</a></homepageurl>"
  "<logourl></logourl>"
  "<shortdesc>"
  "Logitech 160x43 LCD via HID"
  "</shortdesc>"
  "<longdesc>"
    "HID-level driver for the Logitech devices with a 160x43 LCD: G13, G15, G15 v2, G510 and Z-10. "
    "It also reports their G-keys, M-keys and LCD buttons to layouts.<br/>"
    "No driver needs to be installed. On Linux, use <i>Install udev rules</i> in the settings of this plugin. "
    "See <a href=\"https://github.com/LokLakh-s/LCDHost-Revival/blob/master/docs/HARDWARE.md\">docs/HARDWARE.md</a>."
  "</longdesc>"
"</lcdhostplugin>";

static const char hardware_guide_url[] =
        "https://github.com/LokLakh-s/LCDHost-Revival/blob/master/docs/HARDWARE.md";

LH_Lg160x43::LH_Lg160x43() :
    LH_QtPlugin(),
    setup_status_(0),
    timer_id_(0)
{
}

const char *LH_Lg160x43::userInit()
{
    const int info_flags = LH_FLAG_NOSAVE_DATA | LH_FLAG_NOSAVE_LINK | LH_FLAG_NOSOURCE | LH_FLAG_NOSINK;

    setup_status_ = new LH_Qt_QString(this, "Status",
                                      QString("Looking for a %1...").arg(Lg160x43Model::supportedText()),
                                      info_flags, lh_type_string_html);
    connect(new LgUdevSetup(this), SIGNAL(rulesChanged()), this, SLOT(refreshStatus()));
    new LH_Qt_QString(this, "~guide",
                      QString("<a href=\"%1\">Hardware setup guide</a>").arg(hardware_guide_url),
                      info_flags | LH_FLAG_HIDETITLE, lh_type_string_html);

    LH_HidDevice::subscribe(this, SLOT(onlineChanged(LH_HidDevice*,bool)));
    // Notices unplugged devices that could not be opened, and LGS.
    timer_id_ = startTimer(2000);
    return 0;
}

void LH_Lg160x43::userTerm()
{
    if(timer_id_)
    {
        killTimer(timer_id_);
        timer_id_ = 0;
    }
    QList<Lg160x43Device *> devices = devices_.values();
    devices_.clear();
    qDeleteAll(devices);
}

void LH_Lg160x43::timerEvent(QTimerEvent *ev)
{
    if(ev->timerId() != timer_id_)
        return;
    QSet<int> present_products, lcd_products;
    foreach(LH_HidDevice *hd, known_)
    {
        if(!hd->present())
            continue;
        present_products.insert(hd->product_id());
        if(roles_.value(hd) & RoleLcd)
            lcd_products.insert(hd->product_id());
    }
    unrecognized_ = (present_products - lcd_products).values();
    std::sort(unrecognized_.begin(), unrecognized_.end());
    refreshStatus();
}

// What a HID device carries, from its report descriptor. Without one (a
// device that cannot be opened on Windows), the vendor-defined collection
// FF00:0000 is assumed to carry everything, as on the devices known so far.
int LH_Lg160x43::roles(LH_HidDevice *hd, const Lg160x43Model *model)
{
    int result = 0;
    LH_HidReportInfo info = hd->reportInfo();
    if(info.isValid())
    {
        if(info.has(LH_HidReportInfo::Output, model->lcd_report_id))
            result |= RoleLcd;
        if(model->key_report_id && info.has(LH_HidReportInfo::Input, model->key_report_id))
            result |= RoleKeys;
    }
    else if(hd->hasUsage(0xFF00, 0x0000))
        result = RoleLcd | RoleKeys;
    return result;
}

void LH_Lg160x43::onlineChanged(LH_HidDevice *hd, bool b)
{
    const Lg160x43Model *model = Lg160x43Model::find(hd->vendor_id(), hd->product_id());
    if(!model)
        return;
    if(!known_.contains(hd))
        known_.append(hd);

    if(b)
    {
        if(!roles_.contains(hd))
            roles_.insert(hd, roles(hd, model));
        if((roles_.value(hd) & RoleLcd) && !devices_.contains(hd))
        {
            // Only offer the LCD when LCDHost can use it, and say why not.
            OpenError error = { 0, QString() };
            if(hd->open(&error.text, &error.code))
            {
                errors_.remove(hd);
                addDevice(hd, model);
            }
            else
                errors_.insert(hd, error);
        }
    }
    else
        removeDevice(hd);

    attachKeys();
    refreshStatus();
}

QString LH_Lg160x43::uniqueDevid(LH_HidDevice *hd, const Lg160x43Model *model) const
{
    QString base = QString("Lg160x43-%1").arg(model->product_id, 4, 16, QChar('0'));
    if(!hd->serial_text().isEmpty())
        base += "-" + hd->serial_text();
    QString devid = base;
    for(int n = 2; ; ++n)
    {
        bool used = false;
        foreach(Lg160x43Device *dev, devices_)
            used = used || dev->devid() == devid;
        if(!used)
            return devid;
        devid = QString("%1-%2").arg(base).arg(n);
    }
}

void LH_Lg160x43::addDevice(LH_HidDevice *hd, const Lg160x43Model *model)
{
    Lg160x43Device *dev = new Lg160x43Device(model, hd, uniqueDevid(hd, model), this);
    connect(dev, SIGNAL(openChanged()), this, SLOT(refreshStatus()));
    devices_.insert(hd, dev);
}

void LH_Lg160x43::removeDevice(LH_HidDevice *hd)
{
    if(Lg160x43Device *dev = devices_.take(hd))
        delete dev;
}

// Lets each LCD report the keys of its HID interface: the same HID device on
// Linux, possibly another top-level collection on Windows.
void LH_Lg160x43::attachKeys()
{
    foreach(Lg160x43Device *dev, devices_)
    {
        LH_HidDevice *keys = 0;
        if(dev->model()->key_report_id)
        {
            const QByteArray interface_path = dev->lcd()->interfacePath();
            foreach(LH_HidDevice *hd, known_)
            {
                if((roles_.value(hd) & RoleKeys) && hd->interfacePath() == interface_path &&
                        (!keys || hd == dev->lcd()) && hd->online())
                    keys = hd;
            }
        }
        dev->setKeys(keys);
    }
}

void LH_Lg160x43::refreshStatus()
{
    QStringList lines;

    // Forget the devices that are gone; they are published again if they return.
    foreach(LH_HidDevice *hd, known_)
    {
        if(!hd->present() && !devices_.contains(hd))
        {
            known_.removeOne(hd);
            roles_.remove(hd);
            errors_.remove(hd);
        }
    }

    QStringList ready;
    foreach(Lg160x43Device *dev, devices_)
        ready << QString("%1 ready%2.").arg(dev->model()->name)
                 .arg(dev->isOpen() ? " and in use by LCDHost" : ": select it in the Output tab");
    ready.sort();
    lines << ready;

    foreach(LH_HidDevice *hd, known_)
    {
        if(!hd->present() || devices_.contains(hd) || !errors_.contains(hd))
            continue;
        const Lg160x43Model *model = Lg160x43Model::find(hd->vendor_id(), hd->product_id());
        const OpenError error = errors_.value(hd);
#ifdef Q_OS_LINUX
        if(error.code == EACCES || error.code == EPERM)
        {
            lines << QString("%1 found, but LCDHost has no permission to use it: %2.")
                     .arg(model->name).arg(LgUdevSetup::permissionHint());
            continue;
        }
#endif
        lines << QString("%1 found, but LCDHost cannot open it (%2).")
                 .arg(model->name).arg(error.text.toHtmlEscaped());
    }
    foreach(int product_id, unrecognized_)
    {
        lines << QString("%1 found, but its LCD interface was not recognized: "
                         "please report it with the hardware test report form.")
                 .arg(Lg160x43Model::find(0x046D, product_id)->name);
    }

#ifdef Q_OS_WIN
    if(FindWindowA("Logitech LCD Monitor Window", "LCDMon") || FindWindowA("QWidget", "LCore"))
        lines << "Logitech Gaming Software is running and also draws on the LCD: quit it.";
#endif

    if(lines.isEmpty())
        lines << QString("No %1 connected.").arg(Lg160x43Model::supportedText());

    const QString text = lines.join("<br/>");
    if(setup_status_ && setup_status_->value() != text)
    {
        qDebug("Lg160x43: %s", qPrintable(text));
        setup_status_->setValue(text);
    }
}
