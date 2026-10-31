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

#include "LH_LgBacklight.h"
#include "LgBacklightDevice.h"
#include "LH_HidDevice.h"
#include "LH_HidReportInfo.h"

#include <QDebug>
#include <QtAlgorithms>
#include <algorithm>
#include <QStringList>
#include <QTimerEvent>

#include <errno.h>

LH_PLUGIN(LH_LgBacklight)

char __lcdhostplugin_xml[] =
"<?xml version=\"1.0\"?>"
"<lcdhostplugin>"
  "<id>Backlight</id>"
  "<rev>" STRINGIZE(REVISION) "</rev>"
  "<api>" STRINGIZE(LH_API_MAJOR) "." STRINGIZE(LH_API_MINOR) "</api>"
  "<ver>" "r" STRINGIZE(REVISION) "</ver>"
  "<author>Johan \"SirReal\" Lindh</author>"
  "<homepageurl><a href=\"http://www.linkdata.se/software/lcdhost\">Link Data Stockholm</a></homepageurl>"
  "<logourl></logourl>"
  "<shortdesc>"
  "Logitech device backlight control via HID"
  "</shortdesc>"
  "<longdesc>"
"<p>HID-level backlight control for Logitech keyboards: colour on the G13, G19 and G510, "
"off, dim or bright on the G11 and G15. No driver needs to be installed; on Linux, install the "
"LCDHost udev rules (see <a href=\"https://github.com/LokLakh-s/LCDHost-Revival/blob/master/docs/HARDWARE.md\">docs/HARDWARE.md</a>).</p>"
"<p>To set the backlight for all connected devices from a layout, create a <i>data source</i> named "
"<tt>/plugin/backlight/all/set</tt> using the link button <img src=\":/LH_Backlight/source.png\"></img> at any color item.</p>"
  "</longdesc>"
"</lcdhostplugin>";

const char *LH_LgBacklight::userInit()
{
    if( const char *err = LH_QtPlugin::userInit() ) return err;
    status_ = new LH_Qt_QString( this, "Status", QString("Looking for a %1...").arg(LgBacklightModel::supportedText()),
                                 LH_FLAG_NOSAVE_DATA|LH_FLAG_NOSAVE_LINK|LH_FLAG_NOSOURCE|LH_FLAG_NOSINK, lh_type_string_html );
    devselect_ = new LH_Qt_QStringList( this, "Device", QStringList(), LH_FLAG_NOSAVE_DATA|LH_FLAG_NOSAVE_LINK|LH_FLAG_NOSOURCE|LH_FLAG_NOSINK|LH_FLAG_HIDDEN );
    devselect_->setHelp( "The device which backlight you want to control." );
    connect( devselect_, SIGNAL(changed()), this, SLOT(changeDev()) );

    devcolor_ = new LH_Qt_QColor( this, "Color", Qt::white, LH_FLAG_NOSAVE_DATA|LH_FLAG_NOSAVE_LINK|LH_FLAG_HIDDEN );
    devcolor_->setHelp("The color you want to see used as backlight.");
    connect( devcolor_, SIGNAL(changed()), this, SLOT(changeColor()) );

    allcolor_ = new LH_Qt_QColor( this, "SetAllColor", Qt::white, LH_FLAG_NOSAVE_DATA|LH_FLAG_NOSAVE_LINK|LH_FLAG_HIDDEN|LH_FLAG_NOSOURCE );
    allcolor_->setSubscribePath("/plugin/backlight/all/set");
    connect( allcolor_, SIGNAL(changed()), this, SLOT(setAllColor()) );

    poll_interval_ = new LH_Qt_int(this, "Poll interval", 500, 100, 5000);
    poll_interval_->setHelp("Interval in milliseconds to check for device color changes");
    connect(poll_interval_, SIGNAL(changed()), this, SLOT(pollIntervalChanged()));

    LH_HidDevice::subscribe(this, SLOT(onlineChanged(LH_HidDevice*,bool)));
    pollIntervalChanged();
    return 0;
}

void LH_LgBacklight::userTerm()
{
    if(poll_timer_id_)
    {
        killTimer(poll_timer_id_);
        poll_timer_id_ = 0;
    }
}

void LH_LgBacklight::timerEvent(QTimerEvent *ev) {
    if(ev->timerId() != poll_timer_id_)
        return;
    if(devselect_->index() >= 0 && devselect_->index() < dev_list_.size())
        if(LgBacklightDevice *bld = dev_list_.at(devselect_->index()))
            if (bld->hd()->online())
                devcolor_->setValue(bld->getDeviceColor());
    refreshStatus();
}

void LH_LgBacklight::pollIntervalChanged() {
    if(poll_timer_id_)
        killTimer(poll_timer_id_);
    poll_timer_id_ = startTimer(poll_interval_->value());
}

void LH_LgBacklight::onlineChanged(LH_HidDevice *hd, bool b)
{
    const LgBacklightModel *model = LgBacklightModel::find(hd->vendor_id(), hd->product_id());
    if(!model)
        return;

    if(!b)
    {
        delete devices_.take(hd);
        refreshList();
        return;
    }
    if(devices_.contains(hd))
        return;

    // Only the HID interface (Linux) or top-level collection (Windows) that
    // declares the backlight report. Without a descriptor (a collection that
    // cannot be opened on Windows), only the vendor-defined one.
    LH_HidReportInfo info = hd->reportInfo();
    int report_size = model->report_size;
    if(info.isValid())
    {
        if(!info.has(LH_HidReportInfo::Feature, model->report_id))
            return;
        report_size = info.size(LH_HidReportInfo::Feature, model->report_id);
    }
    else if(!hd->hasUsage(0xFF00, 0x0000))
        return;

    QString error_text;
    int error_code = 0;
    if(!hd->open(&error_text, &error_code))
    {
        errors_.insert(hd, qMakePair(error_code, error_text));
        refreshStatus();
        return;
    }
    errors_.remove(hd);

    LgBacklightDevice *bld = new LgBacklightDevice(hd, model, report_size, this);
    connect(bld, SIGNAL(colorChanged()), this, SLOT(colorChanged()));
    devices_.insert(hd, bld);
    refreshList();
}

void LH_LgBacklight::refreshStatus()
{
    QStringList lines;
    foreach(LH_HidDevice *hd, errors_.keys())
    {
        if(!hd->present())
        {
            errors_.remove(hd);
            continue;
        }
        const LgBacklightModel *model = LgBacklightModel::find(hd->vendor_id(), hd->product_id());
        const QPair<int, QString> error = errors_.value(hd);
#ifdef Q_OS_LINUX
        if(error.first == EACCES || error.first == EPERM)
        {
            lines << QString("%1 found, but LCDHost has no permission to use it: "
                             "install the LCDHost udev rules (see docs/HARDWARE.md).").arg(model->name);
            continue;
        }
#endif
        lines << QString("%1 found, but LCDHost cannot open it (%2).")
                 .arg(model->name).arg(error.second.toHtmlEscaped());
    }
    if(!dev_list_.isEmpty())
        lines.prepend(QString("%1 found.").arg(QStringList(devselect_->list()).join(", ")));
    if(lines.isEmpty())
        lines << QString("No %1 connected.").arg(LgBacklightModel::supportedText());
    const QString text = lines.join("<br/>");
    if(status_ && status_->value() != text)
    {
        qDebug("LgBacklight: %s", qPrintable(text));
        status_->setValue(text);
    }
}

void LH_LgBacklight::colorChanged() {
    if(devselect_->index() >= 0 && devselect_->index() < dev_list_.size())
        if(LgBacklightDevice *bld = dev_list_.at(devselect_->index()))
            devcolor_->setValue(bld->color());
    return;
}

void LH_LgBacklight::refreshList()
{
    LgBacklightDevice *current_bld = 0;
    if(devselect_->index() >= 0 && devselect_->index() < dev_list_.size())
        current_bld = dev_list_.value(devselect_->index());

    dev_list_ = devices_.values();
    std::sort(dev_list_.begin(), dev_list_.end(),
              [](const LgBacklightDevice *a, const LgBacklightDevice *b) {
                  return qstrcmp(a->model()->name, b->model()->name) < 0;
              });

    if(! dev_list_.contains(current_bld))
        current_bld = dev_list_.isEmpty() ? 0 : dev_list_.first();

    int bld_index = 0;
    QStringList sl;
    foreach(LgBacklightDevice *bld, dev_list_)
    {
        if(bld == current_bld)
            bld_index = sl.size();
        sl.append(QString::fromLatin1(bld->model()->name));
    }

    devselect_->setVisible(sl.size() > 0);
    devcolor_->setVisible(sl.size() > 0);
    devselect_->setList(sl);
    devselect_->setIndex(bld_index);
    devcolor_->setValue(current_bld ? current_bld->color() : QColor(Qt::transparent));
    refreshStatus();
    return;
}

void LH_LgBacklight::changeDev()
{
    if(devselect_->index() >= 0 && devselect_->index() < dev_list_.size())
        devcolor_->setValue(dev_list_.at(devselect_->index())->color());
}

void LH_LgBacklight::changeColor()
{
    if(devselect_->index() >= 0 && devselect_->index() < dev_list_.size())
        if(LgBacklightDevice *bld = dev_list_.at(devselect_->index()))
            bld->setColor(devcolor_->value());
}

void LH_LgBacklight::setAllColor()
{
    foreach(LgBacklightDevice *bld, dev_list_)
        bld->setColor(allcolor_->value());
}
