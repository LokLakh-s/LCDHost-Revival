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

#ifndef LH_UDEVRULES_H
#define LH_UDEVRULES_H

#include <QObject>
#include <QProcess>
#include <QString>

#if defined(LH_HID_LIBRARY)
# define LH_UDEVRULES_EXPORT Q_DECL_EXPORT
#else
# define LH_UDEVRULES_EXPORT Q_DECL_IMPORT
#endif

// Linux: installs or removes the LCDHost udev rules (packaging/linux/
// 70-lcdhost.rules, built into lh_hid), which give the user of the local
// session access to the Logitech devices. The commands run as root with
// pkexec, which asks for the administrator password.
class LH_UDEVRULES_EXPORT LH_UdevRules : public QObject
{
    Q_OBJECT

public:
    explicit LH_UdevRules(QObject *parent = 0);

    // True on Linux.
    static bool isSupported();
    // Where the rules are installed, empty if they are not.
    static QString installedPath();
    // The path that install() writes and remove() deletes.
    static QString localPath();
    // The rules themselves.
    static QByteArray rules();

    bool isRunning() const { return process_ != 0; }
    void install();
    void remove();

signals:
    // A message for the user, ready to show.
    void finished(bool ok, const QString &message);

private slots:
    void processFinished(int exit_code, QProcess::ExitStatus exit_status);
    void processError(QProcess::ProcessError error);

private:
    void run(bool install);
    void done(bool ok, const QString &message);

    QProcess *process_;
    bool installing_;
};

#endif // LH_UDEVRULES_H
