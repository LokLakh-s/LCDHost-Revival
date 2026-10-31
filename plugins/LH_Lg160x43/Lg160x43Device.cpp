/*
  Copyright (c) 2009-2016 Johan Lindh <johan@linkdata.se>
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


#include "Lg160x43Device.h"
#include "Lg160x43Model.h"
#include "LH_HidDevice.h"

#include <QImage>
#include <QtDebug>

enum
{
        G15_LCD_OFFSET = 32,	/* offset in output report where pixels start */
        G15_LCD_HEIGHT = 43,	/* LCD vertical height in pixels */
        G15_LCD_WIDTH = 160,	/* LCD horizontal width in pixels */
        G15_BUFFER_LEN = 0x03e0	/* total length of the HID output report */
};

Lg160x43Device::Lg160x43Device(const Lg160x43Model *model, LH_HidDevice *lcd,
                               const QString &devid, LH_QtPlugin *drv) :
    LH_QtDevice(drv),
    model_(model),
    lcd_(lcd),
    keys_(0),
    pressed_(0),
    opened_(false)
{
    setDevid(devid);
    setName(QString::fromLatin1(model->name));
    setSize(G15_LCD_WIDTH, G15_LCD_HEIGHT);
    setDepth(1);
    setAutoselect(true);
    arrive();
}

Lg160x43Device::~Lg160x43Device()
{
    setKeys(0);
}

void Lg160x43Device::setKeys(LH_HidDevice *hd)
{
    if(hd == keys_)
        return;
    if(keys_)
    {
        disconnect(keys_, 0, this, 0);
        keys_->listen(false);
        // Release what was held down, so that layouts do not see stuck keys.
        for(int i = 0; i < model_->key_count && i < 64; ++i)
            if(pressed_ & (quint64(1) << i))
                sendKey(i, false);
        pressed_ = 0;
    }
    keys_ = hd;
    if(keys_)
    {
        connect(keys_, SIGNAL(inputReport(QByteArray)), this, SLOT(inputReport(QByteArray)));
        keys_->listen(true);
    }
}

void Lg160x43Device::inputReport(const QByteArray &report)
{
    quint64 pressed = 0;
    if(!model_->decodeKeys(report, &pressed) || pressed == pressed_)
        return;
    for(int i = 0; i < model_->key_count && i < 64; ++i)
    {
        quint64 mask = quint64(1) << i;
        if((pressed & mask) != (pressed_ & mask))
            sendKey(i, (pressed & mask) != 0);
    }
    pressed_ = pressed;
}

void Lg160x43Device::sendKey(int index, bool down)
{
    lh_device_input di;
    di.devid = lh_dev()->devid;
    di.control = model_->keys[index].name;
    di.item = index + 1; // must not be zero
    di.flags = lh_df_button | (down ? lh_df_down : lh_df_up);
    di.value = down ? 0xFFFF : 0;
    // Logged so that hardware testers can check the key mapping.
    qDebug("Lg160x43: %s: %s %s", model_->name, di.control, down ? "pressed" : "released");
    callback(lh_cb_input, (void*) &di);
}

const char* Lg160x43Device::open()
{
    opened_ = true;
    emit openChanged();
    return NULL;
}

const char* Lg160x43Device::close()
{
    opened_ = false;
    emit openChanged();
    return NULL;
}

static void make_output_report(unsigned char *lcd_buffer, unsigned char const *data)
{
    unsigned int output_offset = G15_LCD_OFFSET;
    unsigned int base_offset = 0;

    memset( lcd_buffer, 0, G15_LCD_OFFSET );
    for( int row=0; row<6; ++row )
    {
        for( int col=0; col<G15_LCD_WIDTH; ++col)
        {
            unsigned char b;
            unsigned int bit = col % 8;

            b = (((data[base_offset                        ] << bit) & 0x80) >> 7) |
                (((data[base_offset +  G15_LCD_WIDTH/8     ] << bit) & 0x80) >> 6) |
                (((data[base_offset + (G15_LCD_WIDTH/8 * 2)] << bit) & 0x80) >> 5);

            if( row < 5 )
            {
                b |= (((data[base_offset + (G15_LCD_WIDTH/8 * 3)] << bit) & 0x80) >> 4) |
                     (((data[base_offset + (G15_LCD_WIDTH/8 * 4)] << bit) & 0x80) >> 3) |
                     (((data[base_offset + (G15_LCD_WIDTH/8 * 5)] << bit) & 0x80) >> 2) |
                     (((data[base_offset + (G15_LCD_WIDTH/8 * 6)] << bit) & 0x80) >> 1) |
                     (((data[base_offset + (G15_LCD_WIDTH/8 * 7)] << bit) & 0x80) >> 0);
            }

#ifdef Q_OS_MAC
            lcd_buffer[ output_offset++ ] = b;
#else
            lcd_buffer[ output_offset++ ] = ~b;
#endif
            if( bit == 7 ) base_offset++;
        }
        base_offset += G15_LCD_WIDTH - (G15_LCD_WIDTH / 8);
    }
}

const char* Lg160x43Device::render_qimage(QImage *img)
{
    if(img && lcd_ && lcd_->online())
    {
        unsigned char buffer[G15_BUFFER_LEN];
        if( img->depth() == 1 )
            make_output_report( buffer, img->bits() );
        else
        {
            QImage tmp = img->convertToFormat(QImage::Format_Mono,Qt::ThresholdDither|Qt::NoOpaqueDetection);
            make_output_report( buffer, tmp.bits() );
        }
        buffer[0] = (unsigned char) model_->lcd_report_id;
        lcd_->write(QByteArray((char*)buffer, sizeof(buffer)));
    }
    return 0;
}
