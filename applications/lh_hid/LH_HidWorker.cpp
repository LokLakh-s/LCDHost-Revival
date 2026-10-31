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

#include "LH_HidWorker.h"
#include "LH_HidRequest.h"
#include "LH_HidDevice.h"
#include "LH_HidPublisher.h"
#include "LH_HidThread.h"
#include <hidapi.h>

#include <errno.h>

#include <QThread>
#include <QEvent>
#include <QCoreApplication>
#include <QFile>
#include <QSet>
#include <QStringList>
#include <QTimerEvent>
#include <QDebug>

// Input reports are read this often while a driver listens to a device. The
// operating system queues the reports in between.
static const int poll_interval_ms = 20;

QAtomicPointer<LH_HidWorker> LH_HidWorker::instance_ = 0;
int LH_HidWorker::event_publish_ = -1;

# define HIDINSTANCE static_cast<LH_HidWorker *>(LH_HidWorker::instance_.loadAcquire())

bool LH_HidWorker::subscribe(QObject *recipient, const char *slot)
{
    LH_HidPublisher *publisher = new LH_HidPublisher(recipient);
    if(connect(publisher, SIGNAL(onlineChanged(LH_HidDevice*,bool)), recipient, slot, Qt::QueuedConnection))
    {
        connect(HIDINSTANCE, SIGNAL(publish(LH_HidDevice*)), publisher, SLOT(publish(LH_HidDevice*)));
        connect(HIDINSTANCE, SIGNAL(publishStart()), publisher, SLOT(publishStart()));
        QCoreApplication::postEvent(HIDINSTANCE, new QEvent((QEvent::Type) event_publish_));
        connect(HIDINSTANCE, SIGNAL(onlineChanged(LH_HidDevice*,bool)), recipient, slot);
        return true;
    }
    delete publisher;
    qCritical("HID: subscribe failed: %s::%s",
              recipient->metaObject()->className(),
              (slot && isdigit(*slot)) ? slot + 1 : slot);
    return false;
}

LH_HidWorker::LH_HidWorker() :
    QObject(0),
    timer_id_(0),
    poll_timer_id_(0),
    hidapi_inited_(false)
{
    if(instance_.testAndSetOrdered(0, this))
    {
        qDebug("Starting HID\n");
        event_publish_ = QEvent::registerEventType(event_publish_);
        hidapi_inited_ = !hid_init();
        timer_id_ = startTimer(2000);
    }
}

LH_HidWorker::~LH_HidWorker()
{
    if(instance_.testAndSetOrdered(this, 0))
    {
        qDebug("Stopping HID\n");
        if (timer_id_) {
          killTimer(timer_id_);
          timer_id_ = 0;
        }
        if (poll_timer_id_) {
          killTimer(poll_timer_id_);
          poll_timer_id_ = 0;
        }
        stopHidApi();
    }
}

bool LH_HidWorker::checkOnline(LH_HidDevice *hd) const
{
    Q_ASSERT(hd->unused_ == false);
    if(hidapi_inited_ == false || hd->retired_ || hd->device_path_.isEmpty())
        return false;
    if(hd->handle_ == 0)
    {
        hd->handle_ = hid_open_path(hd->device_path());
        if(hd->handle_)
            hid_set_nonblocking((hid_device*) hd->handle_, 1);
    }
    return hd->handle_ != 0;
}

void LH_HidWorker::closeDevice(LH_HidDevice *hd) const
{
    if(hd->handle_)
    {
        hid_close((hid_device*) hd->handle_);
        hd->handle_ = 0;
    }
}

void LH_HidWorker::stopHidApi()
{
    if(hidapi_inited_)
    {
        hidapi_inited_ = false;
        // qDebug("HID: stopping hidapi\n");
        foreach(LH_HidDevice *hd, findChildren<LH_HidDevice *>())
            closeDevice(hd);
        hid_exit();
    }
}

void LH_HidWorker::setOnline(LH_HidDevice *hd, bool state) const
{
    if(! state)
        closeDevice(hd);

    if(hd->online_ != state)
    {
#if 0
        qDebug("HID: %s '%s': %s",
               hd->device_path(),
               hd->product_text().toLocal8Bit().constData(),
               state ? "online" : "offline"
               );
#endif
        hd->online_ = state;
        emit onlineChanged(hd, state);
        emit hd->onlineChanged(state);
    }
}


int LH_HidWorker::readDescriptor(LH_HidDevice *hd) const
{
    if(hd->descriptor_.isEmpty() && !hd->retired_)
    {
#if defined(Q_OS_LINUX)
        // hidapi uses hidraw nodes (/dev/hidrawN) on Linux. Their report
        // descriptor is in sysfs, readable even without access to the node.
        QByteArray node = hd->device_path_.mid(hd->device_path_.lastIndexOf('/') + 1);
        QFile file(QString::fromLatin1("/sys/class/hidraw/%1/device/report_descriptor")
                   .arg(QString::fromLocal8Bit(node)));
        if(!node.isEmpty() && file.open(QIODevice::ReadOnly))
            hd->descriptor_ = file.read(4096);
#elif defined(HID_API_VERSION)
# if HID_API_VERSION >= HID_API_MAKE_VERSION(0, 14, 0)
        // On Windows, hidapi rebuilds the descriptor of the top-level collection.
        bool was_open = hd->handle_ != 0;
        if(hidapi_inited_ && !was_open)
            hd->handle_ = hid_open_path(hd->device_path());
        if(hd->handle_)
        {
            unsigned char buffer[HID_API_MAX_REPORT_DESCRIPTOR_SIZE];
            int size = hid_get_report_descriptor((hid_device*) hd->handle_, buffer, sizeof(buffer));
            if(size > 0)
                hd->descriptor_ = QByteArray((const char *) buffer, size);
            if(!was_open)
                closeDevice(hd);
        }
# endif
#endif
    }
    return hd->descriptor_.size();
}

void LH_HidWorker::processRequest(LH_HidRequest *hidreq)
{
    LH_HidDevice *hd = hidreq->device();
    int retv = -1;
    const char *error_source = "hid_open_path";
    errno = 0;

    switch(hidreq->request())
    {
    case LH_HidRequest::HidReportDescriptor:
        // Does not mark the device as used: drivers read the descriptor to
        // find out whether a device is theirs at all.
        error_source = "no report descriptor";
        retv = readDescriptor(hd);
        if(retv > 0)
            hidreq->report() = hd->descriptor_;
        else
            retv = -1;
        break;

    case LH_HidRequest::HidUnlisten:
        if(hd->listeners_ > 0)
            -- hd->listeners_;
        updatePolling();
        retv = 0;
        break;

    default:
        hd->unused_ = false;
        if(! checkOnline(hd))
            break;
        switch(hidreq->request())
        {
        case LH_HidRequest::HidCheckOnline:
        case LH_HidRequest::HidOpen:
            retv = 0;
            break;
        case LH_HidRequest::HidListen:
            ++ hd->listeners_;
            updatePolling();
            retv = 0;
            break;
        case LH_HidRequest::HidRead:
            error_source = "hid_read";
            retv = hid_read((hid_device*) hd->handle_, hidreq->data(), hidreq->size());
            hidreq->report().truncate(retv < 0 ? 0 : retv);
            break;
        case LH_HidRequest::HidWrite:
            error_source = "hid_write";
            retv = hid_write((hid_device*) hd->handle_, hidreq->constData(), hidreq->size());
            if(retv > -1 && (size_t)retv != hidreq->size())
                qDebug("HID: %s '%s': short write (%d of %u)",
                       hd->device_path(),
                       hd->product_text_.toLocal8Bit().constData(),
                       retv, (unsigned int)hidreq->size()
                       );
            break;
        case LH_HidRequest::HidReadFeature:
            error_source = "hid_get_feature_report";
            retv = hid_get_feature_report((hid_device*) hd->handle_, hidreq->data(), hidreq->size());
            hidreq->report().truncate(retv < 0 ? 0 : retv);
            break;
        case LH_HidRequest::HidWriteFeature:
            error_source = "hid_send_feature_report";
            retv = hid_send_feature_report((hid_device*) hd->handle_, hidreq->constData(), hidreq->size());
            break;
        default:
            error_source = "unknown_request_type";
            break;
        }

        if(retv < 0)
            closeDevice(hd);
        break;
    }

    hidreq->result_code_ = retv;
    if(retv < 0)
    {
        hidreq->setError(error_source, errno);
        if(hidreq->request() == LH_HidRequest::HidOpen)
        {
            // hidapi describes why the device could not be opened.
            const wchar_t *reason = hid_error(0);
            if(reason && *reason)
                hidreq->setError(QString::fromWCharArray(reason).toLocal8Bit().constData());
        }
    }

    if(hidreq->semaphore_.tryAcquire())
    {
        if(hidreq->error())
            qDebug("HID: abandoned %s", hidreq->error());
        delete hidreq;
    }
    else
        hidreq->semaphore_.release();

    return;
}

void LH_HidWorker::updatePolling()
{
    bool needed = false;
    foreach(LH_HidDevice *hd, findChildren<LH_HidDevice *>())
    {
        if(hd->listeners_ > 0)
        {
            needed = true;
            break;
        }
    }
    if(needed && !poll_timer_id_)
        poll_timer_id_ = startTimer(poll_interval_ms);
    else if(!needed && poll_timer_id_)
    {
        killTimer(poll_timer_id_);
        poll_timer_id_ = 0;
    }
}

void LH_HidWorker::pollInput()
{
    unsigned char buffer[256];
    foreach(LH_HidDevice *hd, findChildren<LH_HidDevice *>())
    {
        // A device closed after an error is reopened by the next enumeration.
        if(hd->listeners_ <= 0 || !hd->online_ || !hd->handle_)
            continue;
        // Devices are open in non-blocking mode: hid_read() returns 0 when
        // no report is waiting. Bound the loop so one device cannot starve
        // the others.
        for(int i = 0; i < 64; ++i)
        {
            int size = hid_read((hid_device*) hd->handle_, buffer, sizeof(buffer));
            if(size < 0)
            {
                closeDevice(hd);
                break;
            }
            if(size == 0)
                break;
            emit hd->inputReport(QByteArray((const char *) buffer, size));
        }
    }
}

void LH_HidWorker::enumerateDevices()
{
    QSet<LH_HidDevice *> enumerated_set;
    QSet<LH_HidDevice *> created_set;

    if(struct hid_device_info *hdi_head = hid_enumerate(0, 0))
    {
        for(struct hid_device_info *hdi = hdi_head; hdi; hdi = hdi->next)
        {
            if(!(hdi->path && *hdi->path))
                continue;

            QByteArray device_path(hdi->path);
            const QString serial_text = QString::fromWCharArray(hdi->serial_number);
            LH_HidDevice *hd = 0;
            foreach(LH_HidDevice *child, findChildren<LH_HidDevice *>(QString::fromUtf8(device_path)))
            {
                if(child->retired_)
                    continue;
                if(child->vendor_id() == (hdi->vendor_id & 0xFFFF) &&
                        child->product_id() == (hdi->product_id & 0xFFFF) &&
                        child->serial_text() == serial_text)
                {
                    hd = child;
                    break;
                }
                // Another device got the path (Linux reuses hidraw numbers):
                // the old one is offline for good, drivers get a new object.
                child->retired_ = true;
                closeDevice(child);
            }

            if(hd == 0)
            {
                hd = new LH_HidDevice(
                            device_path,
                            hdi->vendor_id,
                            hdi->product_id,
                            hdi->release_number,
                            serial_text,
                            QString::fromWCharArray(hdi->manufacturer_string),
                            QString::fromWCharArray(hdi->product_string),
                            hdi->usage_page,
                            hdi->usage,
                            this
                            );
                created_set.insert(hd);
            }
            else if(enumerated_set.contains(hd) && created_set.contains(hd))
            {
                // On Linux, hidapi lists each top-level collection of an
                // interface with the same path. Record the usages until the
                // device is published; they do not change afterwards.
                int usage = ((hdi->usage_page & 0xFFFF) << 16) | (hdi->usage & 0xFFFF);
                if(!hd->usages_.contains(usage))
                    hd->usages_.append(usage);
            }
            enumerated_set.insert(hd);
        }
        hid_free_enumeration(hdi_head);
    }
    // hid_enumerate() also returns NULL when no HID device is present: the
    // known devices are then simply offline.

    foreach(LH_HidDevice *hd, findChildren<LH_HidDevice *>())
    {
        bool is_present = enumerated_set.contains(hd);
        bool is_online = is_present;
        hd->present_.storeRelaxed(is_present ? 1 : 0);
        if(is_online && ! hd->unused_)
            is_online = checkOnline(hd);
        setOnline(hd, is_online);
    }
}

void LH_HidWorker::timerEvent(QTimerEvent *ev)
{
    if(poll_timer_id_ && ev->timerId() == poll_timer_id_)
    {
        if(hidapi_inited_)
            pollInput();
        return;
    }
    if(timer_id_ && ev->timerId() == timer_id_)
    {
        if (QCoreApplication::startingUp() || QCoreApplication::closingDown())
            return;
        if(! hidapi_inited_)
        {
            hidapi_inited_ = !hid_init();
            if(!hidapi_inited_)
                qDebug("HID: failed to start hidapi");
        }
        else
        {
            enumerateDevices();
        }
    }
    return;
}

void LH_HidWorker::customEvent(QEvent *ev)
{
    if(ev->type() == LH_HidRequestEvent::type())
    {
        processRequest((static_cast<LH_HidRequestEvent *>(ev))->request());
        return;
    }

    if(ev->type() == event_publish_)
    {
        emit publishStart();
        foreach(LH_HidDevice *hd, findChildren<LH_HidDevice *>())
            emit publish(hd);
        emit publish(0);
        return;
    }
    return;
}
