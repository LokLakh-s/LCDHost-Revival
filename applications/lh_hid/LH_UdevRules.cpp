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

#include "LH_UdevRules.h"

#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

static const char rules_name[] = "70-lcdhost.rules";

LH_UdevRules::LH_UdevRules(QObject *parent) :
    QObject(parent),
    process_(0),
    installing_(false)
{
}

bool LH_UdevRules::isSupported()
{
#ifdef Q_OS_LINUX
    return true;
#else
    return false;
#endif
}

QString LH_UdevRules::localPath()
{
    return QString::fromLatin1("/etc/udev/rules.d/") + QLatin1String(rules_name);
}

QString LH_UdevRules::installedPath()
{
    if(!isSupported())
        return QString();
    // The local rules first, then the places where distribution packages
    // install them.
    static const char *const dirs[] = {
        "/etc/udev/rules.d", "/usr/local/lib/udev/rules.d",
        "/usr/lib/udev/rules.d", "/lib/udev/rules.d"
    };
    for(const char *dir : dirs)
    {
        QString path = QString::fromLatin1("%1/%2").arg(QLatin1String(dir), QLatin1String(rules_name));
        if(QFileInfo::exists(path))
            return path;
    }
    return QString();
}

QByteArray LH_UdevRules::rules()
{
    QFile file(QString::fromLatin1(":/lh_hid/%1").arg(QLatin1String(rules_name)));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

void LH_UdevRules::install()
{
    run(true);
}

void LH_UdevRules::remove()
{
    run(false);
}

void LH_UdevRules::run(bool install)
{
    if(process_ || !isSupported())
        return;

    const QString pkexec = QStandardPaths::findExecutable(QString::fromLatin1("pkexec"));
    if(pkexec.isEmpty())
    {
        done(false, QString::fromLatin1("pkexec was not found: install the rules from a terminal "
                                        "as the hardware setup guide explains."));
        return;
    }
    const QByteArray content = rules();
    if(install && content.isEmpty())
    {
        done(false, QString::fromLatin1("The udev rules are missing from this build of LCDHost."));
        return;
    }

    // One root shell for the whole job, so that the password is asked once.
    // The rules come on its standard input; "$1" is the rules file. udevadm
    // trigger applies them to the devices that are already connected.
    const QString apply = QString::fromLatin1(
                "udevadm control --reload-rules && "
                "udevadm trigger --subsystem-match=usb --subsystem-match=hidraw");
    const QString script = install
            ? QString::fromLatin1("umask 022 && mkdir -p \"$(dirname \"$1\")\" && "
                                  "cat > \"$1.new\" && mv -f \"$1.new\" \"$1\" && %1").arg(apply)
            : QString::fromLatin1("rm -f \"$1\" && %1").arg(apply);

    installing_ = install;
    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(process_, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(process_, SIGNAL(errorOccurred(QProcess::ProcessError)),
            this, SLOT(processError(QProcess::ProcessError)));
    process_->start(pkexec, QStringList() << QString::fromLatin1("/bin/sh") << QString::fromLatin1("-c")
                    << script << QString::fromLatin1("sh") << localPath());
    if(install)
        process_->write(content);
    process_->closeWriteChannel();
}

void LH_UdevRules::processFinished(int exit_code, QProcess::ExitStatus exit_status)
{
    if(!process_)
        return;
    const QString output = QString::fromLocal8Bit(process_->readAll()).trimmed();
    process_->deleteLater();
    process_ = 0;
    if(!output.isEmpty())
        qDebug("udev rules: %s", qPrintable(output));

    if(exit_status != QProcess::NormalExit)
        done(false, QString::fromLatin1("pkexec stopped unexpectedly."));
    else if(exit_code == 0)
        done(true, installing_
             ? QString::fromLatin1("udev rules installed. If a device still cannot be used "
                                   "after a few seconds, unplug it and plug it in again.")
             : QString::fromLatin1("udev rules removed."));
    else if(exit_code == 126)
        done(false, QString::fromLatin1("The administrator password was not given."));
    else if(exit_code == 127)
        // Not authorized, or no authentication agent (pkexec says which).
        done(false, QString::fromLatin1("pkexec could not run the commands as root%1")
             .arg(output.isEmpty() ? QString::fromLatin1(".") : QString::fromLatin1(": ") + output));
    else
        done(false, QString::fromLatin1("The commands failed (exit code %1)%2")
             .arg(exit_code)
             .arg(output.isEmpty() ? QString::fromLatin1(".") : QString::fromLatin1(": ") + output));
}

void LH_UdevRules::processError(QProcess::ProcessError error)
{
    // Other errors also end with finished().
    if(error != QProcess::FailedToStart || !process_)
        return;
    process_->deleteLater();
    process_ = 0;
    done(false, QString::fromLatin1("pkexec could not be started."));
}

void LH_UdevRules::done(bool ok, const QString &message)
{
    qDebug("udev rules: %s", qPrintable(message));
    emit finished(ok, message);
}
