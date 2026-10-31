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


#include "LgBacklightDevice.h"
#include "LH_HidDevice.h"

#include <QByteArray>
#include <QtGlobal>

static const LgBacklightModel models[] = {
    { 0xC21C, "Logitech G13", LgBacklightModel::Rgb, 7, 5 },
    { 0xC229, "Logitech G19", LgBacklightModel::Rgb, 7, 4 },
    { 0xC22D, "Logitech G510", LgBacklightModel::Rgb, 5, 4 },
    { 0xC22E, "Logitech G510", LgBacklightModel::Rgb, 5, 4 }, // with its USB audio on
    { 0xC222, "Logitech G15", LgBacklightModel::Levels, 2, 4 },
    { 0xC227, "Logitech G15 v2", LgBacklightModel::Levels, 2, 4 },
    { 0xC225, "Logitech G11", LgBacklightModel::Levels, 2, 4 },
};

const LgBacklightModel *LgBacklightModel::find(int vendor_id, int product_id)
{
    if(vendor_id != 0x046D)
        return 0;
    for(unsigned i = 0; i < sizeof(models) / sizeof(models[0]); ++i)
        if(models[i].product_id == product_id)
            return &models[i];
    return 0;
}

const char *LgBacklightModel::supportedText()
{
    return "G11, G13, G15, G19 or G510";
}

LgBacklightDevice::LgBacklightDevice(LH_HidDevice *hd, const LgBacklightModel *model,
                                     int report_size, QObject *parent) :
    QObject(parent),
    hd_(hd),
    model_(model),
    report_size_(report_size),
    color_(Qt::transparent),
    red_max_(110), // 140
    blue_max_(90), // 150
    white_balance_(true)
{
    color_ = getDeviceColor();
}

void LgBacklightDevice::setColor(QColor c)
{
    if(color_ != c && setDeviceColor(c))
    {
        color_ = c;
        emit colorChanged();
    }
}

QColor LgBacklightDevice::getDeviceColor()
{
    if(!hd_->online())
        return QColor(Qt::transparent);

    QByteArray d(hd_->readFeature(model_->report_id, report_size_));
    const unsigned char *report = (const unsigned char *) d.constData();

    if(model_->kind == LgBacklightModel::Levels)
    {
        if(d.size() < 2)
            return QColor(Qt::transparent);
        // The keyboard backlight level, shown as a grey.
        int level = qMin(int(report[1]), 2);
        int v = level * 255 / 2;
        return QColor(v, v, v);
    }

    if(d.size() > 3)
    {
        int r = report[1];
        int g = report[2];
        int b = report[3];
        if (white_balance_) {
            int r2 = r * 255 / red_max_;
            if( r2 > 255 ) r2 = 255;
            int b2 = b * 255 / blue_max_;
            if( b2 > 255 ) b2 = 255;
            if (abs(r2 - g) < 3 && abs(b2 - g) < 3) {
                r = r2;
                b = b2;
            }
        }
        return QColor(r, g, b);
    }
    return QColor(Qt::transparent);
}

// automatic intensity correction
bool LgBacklightDevice::setDeviceColor(const QColor &c)
{
    if(!hd_->online())
        return false;

    QByteArray report(qMax(report_size_, 4), '\0');
    report[0] = (char) model_->report_id;

    if(model_->kind == LgBacklightModel::Levels)
    {
        // Off, dim or bright, from the brightest component of the colour.
        int v = qMax(c.red(), qMax(c.green(), c.blue()));
        report[1] = 1; // keyboard backlight (2 would be the LCD)
        report[2] = (char) (v == 0 ? 0 : (v < 128 ? 1 : 2));
    }
    else
    {
        int r = c.red();
        int g = c.green();
        int b = c.blue();
        if (white_balance_) {
            if (abs(r - g) < 3 && abs(g - b) < 3) {
                r = red_max_ * r / 255;
                b = blue_max_ * b / 255;
            }
        }
        report[1] = (char) r;
        report[2] = (char) g;
        report[3] = (char) b;
    }
    return hd_->writeFeature(report) > 0;
}
