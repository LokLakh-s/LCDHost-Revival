/*
  Copyright (c) 2009-2016 Johan Lindh <johan@linkdata.se>

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


#include "LH_HidDevice.h"
#include "LH_HidWorker.h"
#include "LH_HidRequest.h"

#include <ctype.h>
#include <errno.h>

#include <QCoreApplication>
#include <QMetaObject>
#include <QMetaMethod>
#include <QDebug>

void LH_HidDevice::subscribe(QObject *recipient, const char *slot)
{
    LH_HidWorker::subscribe(recipient, slot);
}

LH_HidDevice::LH_HidDevice(
        const QByteArray &device_path,
        int vendor_id,
        int product_id,
        int device_version,
        const QString &serial_text,
        const QString &vendor_text,
        const QString &product_text,
        int usage_page,
        int usage,
        QObject *parent) :
    QObject(parent),
    handle_(0),
    unused_(true),
    online_(false),
    listeners_(0),
    retired_(false),
    present_(0),
    device_path_(device_path),
    vendor_id_(vendor_id & 0xFFFF),
    product_id_(product_id & 0xFFFF),
    device_version_(device_version & 0xFFFF),
    serial_text_(serial_text),
    vendor_text_(vendor_text),
    product_text_(product_text),
    usage_page_(usage_page & 0xFFFF),
    usage_(usage & 0xFFFF)
{
    setObjectName(QString::fromUtf8(device_path.constData()));
    usages_.append(((usage_page & 0xFFFF) << 16) | (usage & 0xFFFF));
}

bool LH_HidDevice::hasUsage(int usage_page, int usage) const
{
    return usages_.contains(((usage_page & 0xFFFF) << 16) | (usage & 0xFFFF));
}

QByteArray LH_HidDevice::interfacePath() const
{
    QByteArray path(device_path_);
    int col = path.toLower().indexOf("&col");
    if(col > 0)
    {
        int end = col + 4;
        while(end < path.size() && isxdigit((unsigned char) path.at(end)))
            ++end;
        path.remove(col, end - col);
    }
    return path;
}

// Posts a request to the HID thread and waits for its result. Returns false
// on timeout, after which the request belongs to the HID thread.
static bool execute(QObject *worker, LH_HidRequest *hid_data)
{
    QCoreApplication::postEvent(worker, new LH_HidRequestEvent(hid_data));
    if(! hid_data->wait())
    {
        hid_data->setError("timeout");
        qDebug("HID: %s", hid_data->error());
        hid_data->abandon();
        return false;
    }
    return true;
}

bool LH_HidDevice::open(QString *errorText, int *errorCode)
{
    LH_HidRequest *hid_data = new LH_HidRequest(this, LH_HidRequest::HidOpen);
    if(! execute(parent(), hid_data))
    {
        if(errorText)
            *errorText = QString::fromLatin1("no answer from the HID thread");
        if(errorCode)
            *errorCode = 0;
        return false;
    }
    bool ok = hid_data->resultCode() == 0;
    if(errorText)
        *errorText = ok ? QString() : QString::fromLocal8Bit(hid_data->error());
    if(errorCode)
        *errorCode = ok ? 0 : hid_data->errorCode();
    delete hid_data;
    return ok;
}

LH_HidReportInfo LH_HidDevice::reportInfo()
{
    LH_HidRequest *hid_data = new LH_HidRequest(this, LH_HidRequest::HidReportDescriptor, 0);
    if(! execute(parent(), hid_data))
        return LH_HidReportInfo();
    LH_HidReportInfo info(hid_data->report());
    delete hid_data;
    return info;
}

bool LH_HidDevice::listen(bool enable)
{
    LH_HidRequest *hid_data = new LH_HidRequest(
                this, enable ? LH_HidRequest::HidListen : LH_HidRequest::HidUnlisten, 0);
    if(! execute(parent(), hid_data))
        return false;
    bool ok = hid_data->resultCode() == 0;
    delete hid_data;
    return ok;
}

bool LH_HidDevice::online()
{
    if(online_ && (unused_ || handle_))
        return true;
    LH_HidRequest *hid_data = new LH_HidRequest(this, LH_HidRequest::HidCheckOnline);
    QCoreApplication::postEvent(parent(), new LH_HidRequestEvent(hid_data));
    if(! hid_data->wait())
    {
        hid_data->abandon();
        return false;
    }
    int result_code = hid_data->resultCode();
    delete hid_data;
    return result_code == 0;
}

QByteArray LH_HidDevice::read(int max_size)
{
    LH_HidRequest *hid_data = new LH_HidRequest(this, LH_HidRequest::HidRead, max_size);
    QCoreApplication::postEvent(parent(), new LH_HidRequestEvent(hid_data));
    if(! hid_data->wait())
    {
        hid_data->setError("timeout");
        qDebug("HID: %s", hid_data->error());
        hid_data->abandon();
        return QByteArray();
    }
    QByteArray report(hid_data->report());
    if(hid_data->error())
        qDebug("HID: %s", hid_data->error());
    delete hid_data;
    return report;
}

int LH_HidDevice::write(const QByteArray &report)
{
    LH_HidRequest *hid_data = new LH_HidRequest(this, LH_HidRequest::HidWrite, report);
    QCoreApplication::postEvent(parent(), new LH_HidRequestEvent(hid_data));
    if(! hid_data->wait())
    {
        hid_data->setError("timeout");
        qDebug("HID: %s", hid_data->error());
        hid_data->abandon();
        return -1;
    }
    int result_code = hid_data->resultCode();
    if(hid_data->error())
        qDebug("HID: %s", hid_data->error());
    delete hid_data;
    return result_code;
}

QByteArray LH_HidDevice::readFeature(int n, int max_size)
{
    LH_HidRequest *hid_data = new LH_HidRequest(this, LH_HidRequest::HidReadFeature, max_size);
    *(hid_data->data()) = (unsigned char) n;
    QCoreApplication::postEvent(parent(), new LH_HidRequestEvent(hid_data));
    if(! hid_data->wait())
    {
        hid_data->setError("timeout");
        qDebug("HID: %s", hid_data->error());
        hid_data->abandon();
        return QByteArray();
    }
    QByteArray report(hid_data->report());
    if(hid_data->error())
        qDebug("HID: %s", hid_data->error());
    delete hid_data;
    return report;
}

int LH_HidDevice::writeFeature(const QByteArray &report)
{
    LH_HidRequest *hid_data = new LH_HidRequest(this, LH_HidRequest::HidWriteFeature, report);
    QCoreApplication::postEvent(parent(), new LH_HidRequestEvent(hid_data));
    if(! hid_data->wait())
    {
        hid_data->setError("timeout");
        qDebug("HID: %s", hid_data->error());
        hid_data->abandon();
        return -1;
    }
    int result_code = hid_data->resultCode();
    if(hid_data->error())
        qDebug("HID: %s", hid_data->error());
    delete hid_data;
    return result_code;
}
