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

#include "LogitechG19.h"
#include <QDebug>
#include <QCoreApplication>

int LogitechG19::g19_event_ = QEvent::registerEventType();

static const int g19_display_interface = 0;

int LogitechG19::probe(libusb_device *usbdev)
{
    libusb_device_handle *handle = 0;
    int retv = libusb_open(usbdev, &handle);
    if(retv == LIBUSB_SUCCESS)
    {
        libusb_set_auto_detach_kernel_driver(handle, 1);
        retv = libusb_claim_interface(handle, g19_display_interface);
        if(retv == LIBUSB_SUCCESS)
            libusb_release_interface(handle, g19_display_interface);
        libusb_close(handle);
    }
    return retv;
}

QString LogitechG19::errorText(int libusb_error)
{
    switch(libusb_error)
    {
    case LIBUSB_SUCCESS:
        return QString();
#ifdef Q_OS_WIN
    case LIBUSB_ERROR_NOT_SUPPORTED:
    case LIBUSB_ERROR_NOT_FOUND:
        return "G19 found, but its display does not use the WinUSB driver: click Install driver below.";
    case LIBUSB_ERROR_ACCESS:
    case LIBUSB_ERROR_BUSY:
        return "G19 found, but another program is using its display (Logitech Gaming Software?). "
               "LCDHost retries once it is closed.";
#else
    case LIBUSB_ERROR_ACCESS:
        return "G19 found, but LCDHost has no permission to use it: install the LCDHost udev rules, "
               "then unplug and replug the keyboard.";
    case LIBUSB_ERROR_BUSY:
        return "G19 found, but another program is using its display. LCDHost retries once it is closed.";
#endif
    case LIBUSB_ERROR_NO_DEVICE:
        return "The G19 was unplugged.";
    default:
        return QString("G19 found, but LCDHost cannot use it (libusb: %1).")
                .arg(QString::fromLatin1(libusb_error_name(libusb_error)));
    }
}

QString LogitechG19::readyText()
{
    return "G19 ready: select it in the Output tab.";
}

void LIBUSB_CALL LogitechG19::g19_button_cb( struct libusb_transfer * transfer )
{
    if( transfer && transfer->user_data )
    {
        LogitechG19 * g19 = (LogitechG19*) transfer->user_data;
        Q_ASSERT( g19->button_transfer_ == transfer );
        g19->button_completed_ = 1;
        if( ! g19->offline() )
        {
            if( transfer->status == LIBUSB_TRANSFER_COMPLETED || transfer->status == LIBUSB_TRANSFER_TIMED_OUT )
            {
                g19->buttons();
                libusb_submit_transfer( transfer );
                g19->button_completed_ = 0;
            }
        }
    }

    return;
}

LogitechG19::LogitechG19(libusb_context *ctx, libusb_device *usbdev, libusb_device_descriptor *dd , LH_QtObject *parent) :
    LH_QtDevice(parent),
    usb_ctx_(ctx),
    usbdev_(usbdev),
    lcdhandle_(0),
    lcd_if_number_(0),
    menukeys_if_number_(1),
    endpoint_in_(0),
    endpoint_out_(0),
    button_transfer_(0),
    button_completed_(1),
    last_buttons_(0),
    new_buttons_(0)
{
    for( int config_num=0; config_num<dd->bNumConfigurations; ++config_num )
    {
        struct libusb_config_descriptor *conf_desc = 0;
        const struct libusb_endpoint_descriptor *endpoint = 0;

        if(libusb_get_config_descriptor(usbdev_, config_num, &conf_desc) != LIBUSB_SUCCESS)
            continue;
        for(int i=0; i<conf_desc->bNumInterfaces && i<1; i++)
        {
            for (int j=0; j<conf_desc->interface[i].num_altsetting; j++)
            {
#if 0
                qDebug("interface[%d].altsetting[%d]: num endpoints = %d\n",
                       i, j, conf_desc->interface[i].altsetting[j].bNumEndpoints);
                qDebug("   Class.SubClass.Protocol: %02X.%02X.%02X\n",
                       conf_desc->interface[i].altsetting[j].bInterfaceClass,
                       conf_desc->interface[i].altsetting[j].bInterfaceSubClass,
                       conf_desc->interface[i].altsetting[j].bInterfaceProtocol);
#endif
                for(int k=0; k<conf_desc->interface[i].altsetting[j].bNumEndpoints; k++)
                {
                    endpoint = & conf_desc->interface[i].altsetting[j].endpoint[k];
#if 0
                    qDebug("       endpoint[%d].address: %02X\n", k, endpoint->bEndpointAddress);
                    qDebug("           max packet size: %04X\n", endpoint->wMaxPacketSize);
                    qDebug("          polling interval: %02X\n", endpoint->bInterval);
#endif
                    if( endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN )
                        endpoint_in_ = endpoint->bEndpointAddress;
                    else
                        endpoint_out_ = endpoint->bEndpointAddress;
                }
            }
        }
        if( conf_desc ) libusb_free_config_descriptor( conf_desc );
    }
    setDevid(deviceId());
    setName("Logitech G19 LCD (USB)");
    setSize(320,240);
    setDepth(16);
#ifdef Q_OS_WIN
    setAutoselect(false);
#else
    setAutoselect(true);
#endif
    arrive();
}

void LogitechG19::userTerm()
{
    if(usbdev_)
    {
        close();
        leave();
        libusb_unref_device(usbdev_);
        usbdev_ = 0;
    }
}

LogitechG19::~LogitechG19()
{
    userTerm();
}

#define ASSERT_USB(x) if( retv == 0 ) { retv = x; if( retv != 0 ) { qDebug() << #x << libusb_error_name( retv ); } }

const char* LogitechG19::open()
{
    int retv = 0;
    int cfg = -1;

    Q_ASSERT(usbdev_);
    Q_ASSERT(button_transfer_ == 0);
    Q_ASSERT(button_completed_ == 1);
    Q_ASSERT(!lcdhandle_);

    last_buttons_ = new_buttons_ = 0;
    button_transfer_ = 0;
    button_completed_ = 1;

    retv = libusb_open(usbdev_, &lcdhandle_);
    if(retv == LIBUSB_SUCCESS)
    {
        Q_ASSERT(lcdhandle_ != 0);
        libusb_set_auto_detach_kernel_driver(lcdhandle_, 1);
        if(libusb_get_configuration(lcdhandle_, &cfg) || cfg != 1)
            libusb_set_configuration(lcdhandle_, 1);
        retv = libusb_claim_interface(lcdhandle_, lcd_if_number_);
        if(retv == LIBUSB_SUCCESS)
        {
            button_completed_ = 0;
            QCoreApplication::postEvent(this, new QEvent((QEvent::Type)g19_event_));
            emit statusChanged("G19 in use by LCDHost.");
            return NULL;
        }
    }

    if (lcdhandle_) {
        libusb_close(lcdhandle_);
        lcdhandle_ = 0;
    }

    emit statusChanged(errorText(retv));
    const char* error_text = libusb_error_name(retv);
    return error_text ? error_text : "Unknown libusb error";
}

const char* LogitechG19::close()
{
    if(button_transfer_)
    {
        if(!libusb_cancel_transfer(button_transfer_))
            while(!button_completed_)
                libusb_handle_events_completed(usb_ctx_, & button_completed_);
        libusb_free_transfer(button_transfer_);
        button_transfer_ = 0;
    }
    if( lcdhandle_ )
    {
        // libusb_release_interface(lcdhandle_, menukeys_if_number_);
        libusb_release_interface(lcdhandle_, lcd_if_number_);
        libusb_close( lcdhandle_ );
        lcdhandle_ = 0;
        emit statusChanged(readyText());
    }
    return NULL;
}

void LogitechG19::buttons( libusb_transfer * )
{
    if(offline())
        return;

    new_buttons_ &= 0x7FFF;
    if( new_buttons_ != last_buttons_ )
    {
        // send delta
        for( int bit=0; bit<16; ++bit )
        {
            int mask = 1<<bit;
            if( (new_buttons_ & mask) != (last_buttons_ & mask) )
            {
                lh_device_input di;
                di.devid = lh_dev()->devid;
                switch( mask )
                {
                case 0x01: di.control = "App"; break;
                case 0x02: di.control = "Cancel"; break;
                case 0x04: di.control = "Menu"; break;
                case 0x08: di.control = "Ok"; break;
                case 0x10: di.control = "Right"; break;
                case 0x20: di.control = "Left"; break;
                case 0x40: di.control = "Down"; break;
                case 0x80: di.control = "Up"; break;
                default: di.control = "Unknown"; break;
                }
                di.item = bit + 1; // must not be zero
                di.flags = lh_df_button;
                if( new_buttons_ & mask )
                {
                    di.flags |= lh_df_down;
                    di.value = 0xFFFF;
                }
                else
                {
                    di.flags |= lh_df_up;
                    di.value = 0x0;
                }
                qDebug("Lg320x240: G19: %s %s", di.control,
                       (di.flags & lh_df_down) ? "pressed" : "released");
                callback( lh_cb_input, (void*) &di );
            }
        }
        last_buttons_ = new_buttons_;
    }

    return;
}

bool LogitechG19::offline() const
{
    return !(usbdev_ && lcdhandle_ && usb_ctx_);
}

#define G19_BUFFER_LEN ((320*240*2)+512)

const char* LogitechG19::render_qimage(QImage *img)
{
    static unsigned char header[16] = {0x10, 0x0F, 0x00, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x01, 0xEF, 0x00, 0x0F};
    unsigned char lcd_buffer[ G19_BUFFER_LEN ], *p;
    int len = 0;
    int usberr = 0;

    if( offline() || !img ) return NULL;

    lcd_buffer[0] = 0x02;
    memcpy( lcd_buffer, header, sizeof(header) );
    for( int i=16; i<256; ++i ) lcd_buffer[i] = i;
    for( int i=0; i<256; ++i ) lcd_buffer[i+256] = i;
    p = lcd_buffer + 512;
    for( int x = 0; x < 320; ++x )
    {
        for( int y = 0; y < 240; ++y )
        {
            QRgb pix = img->pixel(x,y);
            uchar g = (qGreen(pix) >> 2);
            *p++ = (g<<5) | (qBlue(pix) >> 3);
            *p++ = (qRed(pix)&0xF8) | (g>>3);
        }
    }

    usberr = libusb_bulk_transfer(
            lcdhandle_,
            endpoint_out_,
            lcd_buffer,
            sizeof(lcd_buffer),
            &len,
            250
            );

    if( usberr || len != sizeof(lcd_buffer) )
    {
        userTerm();
        emit statusChanged(errorText(usberr ? usberr : LIBUSB_ERROR_IO));
        qDebug() << "LogitechG19::render_qimage():" << libusb_error_name((libusb_error)usberr);
        deleteLater();
        return libusb_error_name((libusb_error)usberr);
    }

    return NULL;
}


void LogitechG19::customEvent(QEvent *ev)
{
    if(!offline() && ev->type() == g19_event_)
    {
        // struct timeval tv = {0, 1000 * 10};
        int transferred = 0;
        int usb_result = libusb_interrupt_transfer(
                    lcdhandle_,
                    endpoint_in_,
                    (unsigned char*) &new_buttons_,
                    sizeof(new_buttons_),
                    &transferred, 50);
        // int usb_result = libusb_handle_events_timeout_completed(usb_ctx_, &tv, NULL);
        if(usb_result == LIBUSB_SUCCESS || usb_result == LIBUSB_ERROR_TIMEOUT)
        {
            if(usb_result == LIBUSB_SUCCESS)
            {
                buttons();
                Q_ASSERT(transferred == sizeof(new_buttons_));
            }
            QCoreApplication::postEvent(this, new QEvent((QEvent::Type)g19_event_));
            return;
        }
        userTerm();
        emit statusChanged(errorText(usb_result));
        qWarning("LogitechG19: libusb error %s", libusb_error_name(usb_result));
        deleteLater();
        return;
    }
}
