/*
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

#include "LgUdevSetup.h"

#include <QTimer>

#include "LH_QtObject.h"
#include "LH_Qt_QString.h"
#include "LH_UdevRules.h"

LgUdevSetup::LgUdevSetup(LH_QtObject *parent) :
    QObject(parent),
    rules_(0),
    info_(0),
    install_(0),
    remove_(0)
{
    if(!LH_UdevRules::isSupported())
        return;

    const int flags = LH_FLAG_NOSAVE_DATA | LH_FLAG_NOSAVE_LINK | LH_FLAG_NOSOURCE |
            LH_FLAG_NOSINK | LH_FLAG_HIDETITLE;
    rules_ = new LH_UdevRules(this);
    connect(rules_, SIGNAL(finished(bool,QString)), this, SLOT(finished(bool,QString)));
    info_ = new LH_Qt_QString(parent, "~udev_info", QString(), flags, lh_type_string_html);
    install_ = new LH_Qt_QString(parent, "Install udev rules", "Install udev rules",
                                 flags, lh_type_string_button);
    connect(install_, SIGNAL(changed()), this, SLOT(install()));
    remove_ = new LH_Qt_QString(parent, "Remove udev rules", "Remove udev rules",
                                flags, lh_type_string_button);
    connect(remove_, SIGNAL(changed()), this, SLOT(remove()));
    refresh();

    // Also notices rules installed or removed from a terminal.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
    timer->start(5000);
}

QString LgUdevSetup::permissionHint()
{
    return LH_UdevRules::isSupported()
            ? QString("click <i>Install udev rules</i> below")
            : QString("see the hardware setup guide");
}

void LgUdevSetup::refresh()
{
    if(!rules_)
        return;
    const QString path = LH_UdevRules::installedPath();
    QString text = path.isEmpty()
            ? QString("udev rules: not installed (needed to use the devices).")
            : (path == LH_UdevRules::localPath()
               ? QString("udev rules: installed.")
               : QString("udev rules: installed by a package."));
    if(!message_.isEmpty())
        text += "<br/>" + message_.toHtmlEscaped();
    info_->setValue(text);
    const bool running = rules_->isRunning();
    install_->setReadonly(running);
    // Rules installed by a distribution package are not ours to remove.
    remove_->setReadonly(running || path != LH_UdevRules::localPath());
}

void LgUdevSetup::install()
{
    message_ = QString("Waiting for the administrator password...");
    rules_->install();
    refresh();
}

void LgUdevSetup::remove()
{
    message_ = QString("Waiting for the administrator password...");
    rules_->remove();
    refresh();
}

void LgUdevSetup::finished(bool ok, const QString &message)
{
    message_ = message;
    refresh();
    if(ok)
        emit rulesChanged();
}
