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

#ifndef LGUDEVSETUP_H
#define LGUDEVSETUP_H

#include <QObject>
#include <QString>

class LH_QtObject;
class LH_Qt_QString;
class LH_UdevRules;

// The udev rules part of the settings of the Logitech drivers: whether the
// LCDHost udev rules are installed, and buttons to install or remove them.
// Linux only: elsewhere it adds nothing. Shared by the Logitech plugins.
class LgUdevSetup : public QObject
{
    Q_OBJECT

public:
    // Creates the setup items: call it from userInit().
    explicit LgUdevSetup(LH_QtObject *parent);

    // What to tell the user when a device cannot be opened for lack of
    // permission.
    static QString permissionHint();

signals:
    // The rules were installed or removed.
    void rulesChanged();

private slots:
    void install();
    void remove();
    void finished(bool ok, const QString &message);
    void refresh();

private:

    LH_UdevRules *rules_;
    LH_Qt_QString *info_;
    LH_Qt_QString *install_;
    LH_Qt_QString *remove_;
    QString message_;
};

#endif // LGUDEVSETUP_H
