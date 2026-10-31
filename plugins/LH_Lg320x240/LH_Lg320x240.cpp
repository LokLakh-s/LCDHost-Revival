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
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTimerEvent>

#ifdef Q_OS_WIN
# include <QWinEventNotifier>
# include <windows.h>
# include <shellapi.h>
#endif

#include <libusb.h>

#include "LH_HidDevice.h"
#include "LH_HidReportInfo.h"
#include "LH_QtDevice.h"
#include "LH_Qt_QString.h"
#include "LH_Lg320x240.h"
#include "G19Keys.h"
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
    "USB-level driver for Logitech 320x240 LCD displays, such as the G19 and G19s. "
    "It also reports the G-keys, M-keys and LCD keys to layouts.<br/>"
    "On Windows, the display interface of the G19 must use Microsoft's "
    "<a href=\"https://learn.microsoft.com/windows-hardware/drivers/usbcon/winusb\">WinUSB</a> driver: "
    "use <i>Install driver</i> in the settings of this plugin. On Linux, install the LCDHost udev rules. "
    "See <a href=\"https://github.com/LokLakh-s/LCDHost-Revival/blob/master/docs/HARDWARE.md\">docs/HARDWARE.md</a>."
  "</longdesc>"
"</lcdhostplugin>";

static const char hardware_guide_url[] =
        "https://github.com/LokLakh-s/LCDHost-Revival/blob/master/docs/HARDWARE.md";

LH_Lg320x240::LH_Lg320x240() :
    LH_QtPlugin(),
    timer_id_(0),
    usb_ctx_(0),
    setup_status_(0),
    setup_install_driver_(0),
    setup_remove_driver_(0),
    driver_setup_process_(0),
    driver_setup_notifier_(0),
    gkeys_hd_(0),
    gkeys_pressed_(0)
{
}

const char *LH_Lg320x240::userInit()
{
    const int info_flags = LH_FLAG_NOSAVE_DATA | LH_FLAG_NOSAVE_LINK | LH_FLAG_NOSOURCE | LH_FLAG_NOSINK;

    setup_status_ = new LH_Qt_QString(this, "Status", "Looking for a G19...",
                                      info_flags, lh_type_string_html);
#ifdef Q_OS_WIN
    new LH_Qt_QString(this, "~driver_info",
                      "LCDHost needs Microsoft's WinUSB driver on the display interface of the G19. "
                      "The keyboard and its G-keys keep their drivers. Installing it requires "
                      "administrator rights. While it is installed, Logitech Gaming Software "
                      "cannot use the display.",
                      info_flags | LH_FLAG_HIDETITLE, lh_type_string_html);
    setup_install_driver_ = new LH_Qt_QString(this, "Install driver", "Install driver",
                                              info_flags | LH_FLAG_HIDETITLE, lh_type_string_button);
    connect(setup_install_driver_, SIGNAL(changed()), this, SLOT(installDriver()));
    setup_remove_driver_ = new LH_Qt_QString(this, "Remove driver", "Remove driver",
                                             info_flags | LH_FLAG_HIDETITLE, lh_type_string_button);
    connect(setup_remove_driver_, SIGNAL(changed()), this, SLOT(removeDriver()));
#endif
    new LH_Qt_QString(this, "~guide",
                      QString("<a href=\"%1\">Hardware setup guide</a>").arg(hardware_guide_url),
                      info_flags | LH_FLAG_HIDETITLE, lh_type_string_html);

    // The G-keys come through HID, whatever the state of the display.
    LH_HidDevice::subscribe(this, SLOT(hidOnlineChanged(LH_HidDevice*,bool)));

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
    stopGKeys();
    stopDriverSetup();
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

void LH_Lg320x240::hidOnlineChanged(LH_HidDevice *hd, bool online)
{
    if(hd->vendor_id() != 0x046d || hd->product_id() != 0xc229)
        return;
    if(!online)
    {
        if(hd == gkeys_hd_)
            stopGKeys();
        return;
    }
    if(gkeys_hd_)
        return;
    // Windows lists the top-level collections of the interface separately:
    // take the one that declares the key report.
    LH_HidReportInfo info = hd->reportInfo();
    bool has_keys = info.isValid() ? info.has(LH_HidReportInfo::Input, G19Keys::ReportId)
                                   : hd->hasUsage(0xFF00, 0x0000);
    if(!has_keys)
        return;
    gkeys_hd_ = hd;
    gkeys_pressed_ = 0;
    connect(hd, SIGNAL(inputReport(QByteArray)), this, SLOT(gkeysReport(QByteArray)));
    hd->listen(true);
}

void LH_Lg320x240::stopGKeys()
{
    if(!gkeys_hd_)
        return;
    disconnect(gkeys_hd_, 0, this, 0);
    gkeys_hd_->listen(false);
    gkeys_hd_ = 0;
    // Release what was held down, so that layouts do not see stuck keys.
    for(int i = 0; i < G19Keys::count(); ++i)
        if(gkeys_pressed_ & (quint32(1) << i))
            sendGKey(i, false);
    gkeys_pressed_ = 0;
}

void LH_Lg320x240::gkeysReport(const QByteArray &report)
{
    quint32 pressed = 0;
    if(!G19Keys::decode(report, &pressed) || pressed == gkeys_pressed_)
        return;
    for(int i = 0; i < G19Keys::count(); ++i)
    {
        quint32 mask = quint32(1) << i;
        if((pressed & mask) != (gkeys_pressed_ & mask))
            sendGKey(i, (pressed & mask) != 0);
    }
    gkeys_pressed_ = pressed;
}

void LH_Lg320x240::sendGKey(int index, bool down)
{
    lh_device_input di;
    di.devid = LogitechG19::deviceId();
    di.control = G19Keys::name(index);
    di.item = 0x100 + index; // distinct from the LCD keys of LogitechG19
    di.flags = lh_df_button | (down ? lh_df_down : lh_df_up);
    di.value = down ? 0xFFFF : 0;
    // Logged so that hardware testers can check the key mapping.
    qDebug("Lg320x240: G19: %s %s", di.control, down ? "pressed" : "released");
    callback(lh_cb_input, (void*) &di);
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
    // A present device reports its own state; the driver setup changes it.
    if(driver_setup_process_ || findChild<LogitechG19 *>())
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

void LH_Lg320x240::installDriver()
{
    runDriverSetup("install");
}

void LH_Lg320x240::removeDriver()
{
    runDriverSetup("uninstall");
}

#ifdef Q_OS_WIN

void LH_Lg320x240::runDriverSetup(const QString &command)
{
    if(driver_setup_process_)
        return;

    const QString exe = QDir::toNativeSeparators(
                QCoreApplication::applicationDirPath() + "/LCDHostDriverSetup.exe");
    if(!QFileInfo::exists(exe))
    {
        setStatus("The driver installer (LCDHostDriverSetup.exe) is not part of this build. "
                  "Install WinUSB with Zadig instead, see the hardware setup guide.");
        return;
    }

    // The installer replaces the driver of the device: stop using it first.
    closeDevice();

    driver_setup_log_ = QDir::toNativeSeparators(QDir::temp().filePath(
                QString("LCDHostDriverSetup-%1.log").arg(QCoreApplication::applicationPid())));
    QFile::remove(driver_setup_log_);
    const QString parameters = QString("%1 --log \"%2\"").arg(command, driver_setup_log_);

    SHELLEXECUTEINFOW info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = reinterpret_cast<LPCWSTR>(exe.utf16());
    info.lpParameters = reinterpret_cast<LPCWSTR>(parameters.utf16());
    info.nShow = SW_HIDE;
    if(!ShellExecuteExW(&info) || !info.hProcess)
    {
        DWORD error = GetLastError();
        if(error == ERROR_CANCELLED)
            setStatus("Driver setup cancelled: it needs administrator rights.");
        else
            setStatus(QString("Could not start the driver installer (Windows error %1).").arg(error));
        return;
    }

    driver_setup_command_ = command;
    driver_setup_process_ = info.hProcess;
    driver_setup_notifier_ = new QWinEventNotifier(info.hProcess, this);
    connect(driver_setup_notifier_, &QWinEventNotifier::activated, this, &LH_Lg320x240::driverSetupFinished);
    setup_install_driver_->setReadonly(true);
    setup_remove_driver_->setReadonly(true);
    setStatus(command == "install" ? "Installing the WinUSB driver..." : "Removing the WinUSB driver...");
}

void LH_Lg320x240::driverSetupFinished()
{
    if(!driver_setup_process_)
        return;

    DWORD exit_code = 1;
    GetExitCodeProcess(driver_setup_process_, &exit_code);
    stopDriverSetup();

    QString error;
    QFile log(driver_setup_log_);
    if(log.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QStringList lines = QString::fromUtf8(log.readAll()).split('\n', Qt::SkipEmptyParts);
        for(const QString &line : lines)
        {
            qDebug("Lg320x240: driver setup: %s", qPrintable(line.trimmed()));
            if(line.startsWith("error: "))
                error = line.mid(7).trimmed();
        }
        log.close();
        log.remove();
    }

    if(exit_code == 0)
        setStatus(driver_setup_command_ == "install"
                  ? "WinUSB driver installed. LCDHost finds the G19 within a few seconds once it is plugged in."
                  : "WinUSB driver removed. The G19 display uses its previous driver again, if it had one.");
    else
        setStatus(QString("Driver setup failed: %1")
                  .arg(error.isEmpty() ? QString("exit code %1.").arg(exit_code) : error));
}

void LH_Lg320x240::stopDriverSetup()
{
    if(driver_setup_notifier_)
    {
        // Called from the notifier's own signal: defer its deletion.
        driver_setup_notifier_->setEnabled(false);
        driver_setup_notifier_->deleteLater();
        driver_setup_notifier_ = 0;
    }
    if(driver_setup_process_)
    {
        CloseHandle(driver_setup_process_);
        driver_setup_process_ = 0;
    }
    if(setup_install_driver_)
        setup_install_driver_->setReadonly(false);
    if(setup_remove_driver_)
        setup_remove_driver_->setReadonly(false);
}

#else

// Only Windows needs a driver: Linux and macOS use their own USB stack.
void LH_Lg320x240::runDriverSetup(const QString &) {}
void LH_Lg320x240::driverSetupFinished() {}
void LH_Lg320x240::stopDriverSetup() {}

#endif
