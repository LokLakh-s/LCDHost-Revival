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

#ifndef LH_HIDDEVICE_H
#define LH_HIDDEVICE_H

#include <QObject>
#include <QAtomicInt>
#include <QByteArray>
#include <QList>
#include <QString>

#include "LH_HidReportInfo.h"

#if defined(LH_HID_LIBRARY)
# define LH_HID_EXPORT Q_DECL_EXPORT
#else
# define LH_HID_EXPORT Q_DECL_IMPORT
#endif

class LH_HidWorker;

class LH_HID_EXPORT LH_HidDevice : public QObject
{
    Q_OBJECT

public:
    // slot signature must match "onlineChanged(LH_HidDevice*,bool)"
    static void subscribe(QObject *recipient, const char *slot);

    const char *device_path() const { return device_path_.constData(); }
    int vendor_id() const { return vendor_id_; }
    int product_id() const { return product_id_; }
    int device_version() const { return device_version_; }
    QString serial_text() const { return serial_text_; }
    QString vendor_text() const { return vendor_text_; }
    QString product_text() const { return product_text_; }
    int usage_page() const { return usage_page_; }
    int usage() const { return usage_; }
    // True if one of the top-level collections of the device has this usage.
    // On Linux, a device is a whole HID interface and can have several.
    bool hasUsage(int usage_page, int usage) const;
    // The path of the HID interface, the same for all its top-level
    // collections: Windows gives each collection its own path.
    QByteArray interfacePath() const { return interfacePathOf(device_path_); }
    static QByteArray interfacePathOf(const QByteArray &device_path);

    // True while the device is connected, even if it cannot be opened.
    bool present() const { return present_.loadRelaxed() != 0; }
    bool online();
    // Opens the device if it is not open yet. Returns true on success, else
    // false with the reason in errorText and, on Linux, the errno value in
    // errorCode (EACCES: no permission).
    bool open(QString *errorText = 0, int *errorCode = 0);
    // The reports that the report descriptor of the device declares. The
    // descriptor is read from sysfs on Linux, so it is known even when the
    // device cannot be opened.
    LH_HidReportInfo reportInfo();

    QByteArray read(int max_size = 256);
    int write(const QByteArray &report);
    QByteArray readFeature(int n, int max_size = 256);
    int writeFeature(const QByteArray &report);

    // While at least one listener is registered, the input reports of the
    // device are delivered by inputReport(), with the report ID as first byte
    // when the device uses report IDs.
    bool listen(bool enable);

signals:
    void onlineChanged(bool online);
    void inputReport(const QByteArray &report);

protected:
    friend class LH_HidWorker;

    explicit LH_HidDevice(
            const QByteArray &device_path,
            int vendor_id,
            int product_id,
            int device_version,
            const QString &serial_text,
            const QString &vendor_text,
            const QString &product_text,
            int usage_page,
            int usage,
            QObject *parent = 0);

    void *handle_;
    bool unused_;
    bool online_;
    int listeners_;
    bool retired_; // replaced by another device with the same path
    QAtomicInt present_;
    QByteArray descriptor_;
    QList<int> usages_; // (usage page << 16) | usage, frozen once published
    QByteArray device_path_;
    int vendor_id_;
    int product_id_;
    int device_version_;
    QString serial_text_;
    QString vendor_text_;
    QString product_text_;
    int usage_page_;
    int usage_;
};

#endif // LH_HIDDEVICE_H
