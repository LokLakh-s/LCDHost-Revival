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

// Unit tests of the HID code that does not need a device: report descriptor
// parsing, the decoding of the key reports of the Logitech keyboards, and the
// udev rules installer (with fake pkexec and udevadm commands).

#include <QtTest>
#include <QTemporaryDir>

#include "LH_HidDevice.h"
#include "LH_HidReportInfo.h"
#include "LH_UdevRules.h"
#include "Lg160x43Model.h"
#include "G19Keys.h"

static QByteArray bytes(std::initializer_list<int> values)
{
    QByteArray result;
    for(int value : values)
        result.append(char(value));
    return result;
}

// Names of the keys whose bits are set, in table order.
static QStringList pressedKeys(const Lg160x43Model *model, quint64 pressed)
{
    QStringList names;
    for(int i = 0; i < model->key_count; ++i)
        if(pressed & (quint64(1) << i))
            names << QString::fromLatin1(model->keys[i].name);
    return names;
}

static void writeScript(const QString &path, const QByteArray &content)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
    file.close();
    QVERIFY(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
}

static QStringList decode(int product_id, const QByteArray &report)
{
    const Lg160x43Model *model = Lg160x43Model::find(0x046D, product_id);
    quint64 pressed = 0;
    if(!model || !model->decodeKeys(report, &pressed))
        return QStringList() << "(not a key report)";
    return pressedKeys(model, pressed);
}

class TestHid : public QObject
{
    Q_OBJECT

private slots:
    void vendorCollectionWithReportIds()
    {
        // A vendor collection like the one of the G15: keys in, LCD out,
        // LEDs as a feature report.
        LH_HidReportInfo info(bytes({
            0x06, 0x00, 0xFF,       // Usage Page (Vendor 0xFF00)
            0x09, 0x00,             // Usage (0)
            0xA1, 0x01,             // Collection (Application)
            0x15, 0x00,             //   Logical Minimum (0)
            0x26, 0xFF, 0x00,       //   Logical Maximum (255)
            0x75, 0x08,             //   Report Size (8)
            0x85, 0x02,             //   Report ID (2)
            0x95, 0x08,             //   Report Count (8)
            0x09, 0x00, 0x81, 0x02, //   Input
            0x85, 0x03,             //   Report ID (3)
            0x96, 0xDF, 0x03,       //   Report Count (991)
            0x09, 0x00, 0x91, 0x02, //   Output
            0x85, 0x02,             //   Report ID (2)
            0x95, 0x03,             //   Report Count (3)
            0x09, 0x00, 0xB1, 0x02, //   Feature
            0xC0,                   // End Collection
        }));
        QVERIFY(info.isValid());
        QVERIFY(info.usesReportIds());
        QCOMPARE(info.size(LH_HidReportInfo::Input, 2), 9);
        QCOMPARE(info.size(LH_HidReportInfo::Output, 3), 992);
        QCOMPARE(info.size(LH_HidReportInfo::Feature, 2), 4);
        QVERIFY(!info.has(LH_HidReportInfo::Output, 2));
        QVERIFY(!info.has(LH_HidReportInfo::Input, 3));
    }

    void bootKeyboardWithoutReportIds()
    {
        // The boot keyboard of the HID specification (appendix E.6).
        LH_HidReportInfo info(bytes({
            0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7,
            0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01,
            0x75, 0x08, 0x81, 0x01, 0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01,
            0x29, 0x05, 0x91, 0x02, 0x95, 0x01, 0x75, 0x03, 0x91, 0x01, 0x95, 0x06,
            0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07, 0x19, 0x00, 0x29, 0x65,
            0x81, 0x00, 0xC0,
        }));
        QVERIFY(info.isValid());
        QVERIFY(!info.usesReportIds());
        QCOMPARE(info.size(LH_HidReportInfo::Input, 0), 8);
        QCOMPARE(info.size(LH_HidReportInfo::Output, 0), 1);
        QVERIFY(!info.has(LH_HidReportInfo::Output, 3));
    }

    void pushPopAndLongItems()
    {
        LH_HidReportInfo info(bytes({
            0xFE, 0x02, 0x10, 0xAA, 0xBB, // long item, skipped
            0x75, 0x08, 0x95, 0x02,       // Report Size (8), Report Count (2)
            0xA4,                         // Push
            0x75, 0x01, 0x95, 0x04,       //   Report Size (1), Report Count (4)
            0x81, 0x02,                   //   Input: 4 bits
            0xB4,                         // Pop
            0x81, 0x02,                   // Input: 16 bits
        }));
        QVERIFY(info.isValid());
        QCOMPARE(info.size(LH_HidReportInfo::Input, 0), 3);
    }

    void invalidDescriptors()
    {
        QVERIFY(!LH_HidReportInfo().isValid());
        QVERIFY(!LH_HidReportInfo(QByteArray()).isValid());
        QVERIFY(!LH_HidReportInfo(bytes({ 0x75, 0x08, 0x26, 0xFF })).isValid());
        QVERIFY(!LH_HidReportInfo(bytes({ 0xFE, 0x05, 0x10, 0x00 })).isValid());
    }

    void interfacePaths()
    {
        // Two top-level collections of the G19 HID interface, as Windows
        // names them, and the same interface without collections.
        const QByteArray col1 = "\\\\?\\hid#vid_046d&pid_c229&mi_01&col01#8&2a6c7f1&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}";
        const QByteArray col2 = "\\\\?\\hid#vid_046d&pid_c229&mi_01&col02#8&2a6c7f1&0&0001#{4d1e55b2-f16f-11cf-88cb-001111000030}";
        const QByteArray other = "\\\\?\\HID#VID_046D&PID_C229&MI_01&Col01#8&3b7d802&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}";
        QCOMPARE(LH_HidDevice::interfacePathOf(col1), LH_HidDevice::interfacePathOf(col2));
        QCOMPARE(LH_HidDevice::interfacePathOf(col1),
                 QByteArray("\\\\?\\hid#vid_046d&pid_c229&mi_01#8&2a6c7f1&0"));
        QVERIFY(LH_HidDevice::interfacePathOf(col1) != LH_HidDevice::interfacePathOf(other));
        // Linux and single-collection paths are interfaces already.
        QCOMPARE(LH_HidDevice::interfacePathOf("/dev/hidraw3"), QByteArray("/dev/hidraw3"));
        const QByteArray single = "\\\\?\\hid#vid_046d&pid_c227#7&1d9f8e2&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}";
        QCOMPARE(LH_HidDevice::interfacePathOf(single), single);
    }

    void models()
    {
        QVERIFY(Lg160x43Model::find(0x046D, 0xC229) == 0); // G19: not 160x43
        QVERIFY(Lg160x43Model::find(0x1234, 0xC222) == 0);
        for(int product_id : { 0xC222, 0xC227, 0xC21C, 0xC22D, 0xC22E, 0x0A07 })
        {
            const Lg160x43Model *model = Lg160x43Model::find(0x046D, product_id);
            QVERIFY(model);
            QCOMPARE(model->lcd_report_id, 3);
            QVERIFY(model->key_count <= 64);
            QSet<QString> names;
            for(int i = 0; i < model->key_count; ++i)
            {
                QVERIFY(model->keys[i].byte > 0);
                QVERIFY(model->keys[i].byte < model->key_report_size);
                names.insert(QString::fromLatin1(model->keys[i].name));
            }
            QCOMPARE(names.size(), model->key_count);
        }
    }

    void g15Keys()
    {
        QCOMPARE(decode(0xC222, bytes({ 2, 0x81, 0x80, 0, 0, 0, 0, 0, 0x80 })),
                 QStringList() << "G1" << "App" << "Softbutton 0" << "Light");
        QCOMPARE(decode(0xC222, bytes({ 2, 0, 0, 0, 0, 0, 0x01, 0x40, 0x40 })),
                 QStringList() << "G18" << "M1" << "MR");
        QCOMPARE(decode(0xC222, bytes({ 2, 0, 0, 0, 0, 0, 0, 0, 0 })), QStringList());
    }

    void g15v2Keys()
    {
        QCOMPARE(decode(0xC227, bytes({ 2, 0x41, 0xA2, 0, 0 })),
                 QStringList() << "G1" << "M1" << "M3" << "App" << "Softbutton 0");
        QCOMPARE(decode(0xC227, bytes({ 2, 0, 0x11, 0, 0 })),
                 QStringList() << "Softbutton 3" << "Light");
    }

    void g13Keys()
    {
        // Bit 23 (backlight on) is not a key; bit 37 is the light key.
        QCOMPARE(decode(0xC21C, bytes({ 1, 128, 128, 0x01, 0, 0x80, 0x01, 0x21 })),
                 QStringList() << "G1" << "App" << "MR" << "Light");
        QCOMPARE(decode(0xC21C, bytes({ 1, 0, 255, 0, 0, 0x20, 0x12, 0x0E })),
                 QStringList() << "G22" << "Softbutton 0" << "Softbutton 3"
                 << "Stick 1" << "Stick 2" << "Stick 3");
    }

    void g510Keys()
    {
        QCOMPARE(decode(0xC22D, bytes({ 3, 0x01, 0x80, 0x82, 0x41 })),
                 QStringList() << "G1" << "G16" << "G18" << "MR" << "App" << "Microphone mute");
        QCOMPARE(decode(0xC22E, bytes({ 3, 0, 0, 0x10, 0x22 })),
                 QStringList() << "M1" << "Softbutton 0" << "Headphone mute");
    }

    void z10Keys()
    {
        QCOMPARE(decode(0x0A07, bytes({ 2, 0, 0, 0x80, 0, 0, 0, 0, 0x80 })),
                 QStringList() << "App" << "Softbutton 1");
    }

    void notKeyReports()
    {
        QCOMPARE(decode(0xC222, bytes({ 3, 0, 0, 0, 0, 0, 0, 0, 0 })),
                 QStringList() << "(not a key report)");
        QCOMPARE(decode(0xC222, bytes({ 2, 0, 0 })),
                 QStringList() << "(not a key report)");
        QCOMPARE(decode(0xC22D, bytes({ 4, 0x04 })),
                 QStringList() << "(not a key report)");
    }

    void g19Keys()
    {
        QSet<QString> names;
        for(int i = 0; i < G19Keys::count(); ++i)
            names.insert(QString::fromLatin1(G19Keys::name(i)));
        QCOMPARE(names.size(), G19Keys::count());

        quint32 pressed = 0xFFFF;
        QVERIFY(G19Keys::decode(bytes({ 2, 0, 0, 0x40 }), &pressed));
        QCOMPARE(pressed, quint32(0));

        QVERIFY(G19Keys::decode(bytes({ 2, 0x01, 0x98, 0x48 }), &pressed));
        QStringList down;
        for(int i = 0; i < G19Keys::count(); ++i)
            if(pressed & (quint32(1) << i))
                down << QString::fromLatin1(G19Keys::name(i));
        QCOMPARE(down, QStringList() << "G1" << "G12" << "M1" << "MR" << "Light");

        QVERIFY(!G19Keys::decode(bytes({ 1, 0x10 }), &pressed));
        QVERIFY(!G19Keys::decode(bytes({ 2, 0x01 }), &pressed));
    }
};

// Runs the install and remove scripts of LH_UdevRules for real, but as the
// current user and on a test copy of the rules file: the fake pkexec checks
// its arguments and swaps the path, the fake udevadm logs its calls.
class TestUdevRules : public QObject
{
    Q_OBJECT

    QTemporaryDir dir_;
    QByteArray old_path_;

    QString rulesPath() const { return dir_.filePath("etc/udev/rules.d/70-lcdhost.rules"); }

    QByteArray udevadmCalls() const
    {
        QFile log(dir_.filePath("udevadm.log"));
        return log.open(QIODevice::ReadOnly) ? log.readAll() : QByteArray();
    }

    bool run(LH_UdevRules &rules, bool install, QString *message)
    {
        QSignalSpy spy(&rules, SIGNAL(finished(bool,QString)));
        if(install)
            rules.install();
        else
            rules.remove();
        if(spy.isEmpty() && !spy.wait(10000))
            return false;
        *message = spy.at(0).at(1).toString();
        return spy.at(0).at(0).toBool();
    }

private slots:
    void initTestCase()
    {
        if(!LH_UdevRules::isSupported())
            QSKIP("udev rules are Linux only");
        QVERIFY(dir_.isValid());
        QVERIFY(QDir(dir_.path()).mkpath("bin"));
        writeScript(dir_.filePath("bin/pkexec"),
                    "#!/bin/sh\n"
                    "[ -n \"$FAKE_PKEXEC_EXIT\" ] && exit \"$FAKE_PKEXEC_EXIT\"\n"
                    "if [ $# -ne 5 ] || [ \"$1\" != /bin/sh ] || [ \"$2\" != -c ] ||\n"
                    "   [ \"$5\" != /etc/udev/rules.d/70-lcdhost.rules ]; then\n"
                    "    echo \"unexpected arguments: $*\"; exit 99\n"
                    "fi\n"
                    "exec /bin/sh -c \"$3\" sh \"$FAKE_RULES\"\n");
        writeScript(dir_.filePath("bin/udevadm"),
                    "#!/bin/sh\necho \"$*\" >> \"$FAKE_UDEVADM_LOG\"\n");
        old_path_ = qgetenv("PATH");
        qputenv("PATH", dir_.filePath("bin").toLocal8Bit() + ":" + old_path_);
        qputenv("FAKE_RULES", rulesPath().toLocal8Bit());
        qputenv("FAKE_UDEVADM_LOG", dir_.filePath("udevadm.log").toLocal8Bit());
    }

    void cleanupTestCase()
    {
        if(!old_path_.isEmpty())
            qputenv("PATH", old_path_);
    }

    void rulesAreBuiltIn()
    {
        const QByteArray rules = LH_UdevRules::rules();
        QVERIFY(rules.contains("TAG+=\"uaccess\""));
        QVERIFY(rules.contains("c229"));
        QVERIFY(rules.contains("hidraw"));
    }

    void installAndRemove()
    {
        LH_UdevRules rules;
        QString message;
        QVERIFY2(run(rules, true, &message), qPrintable(message));
        QVERIFY(!rules.isRunning());
        QFile file(rulesPath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), LH_UdevRules::rules());
        QCOMPARE(int(file.permissions() & (QFileDevice::ReadOther | QFileDevice::WriteOther)),
                 int(QFileDevice::ReadOther));
        QCOMPARE(udevadmCalls(),
                 QByteArray("control --reload-rules\n"
                            "trigger --subsystem-match=usb --subsystem-match=hidraw\n"));

        QVERIFY2(run(rules, false, &message), qPrintable(message));
        QVERIFY(!QFile::exists(rulesPath()));
        QCOMPARE(udevadmCalls().count("--reload-rules"), 2);
    }

    void passwordNotGiven()
    {
        qputenv("FAKE_PKEXEC_EXIT", "126");
        LH_UdevRules rules;
        QString message;
        QVERIFY(!run(rules, true, &message));
        qunsetenv("FAKE_PKEXEC_EXIT");
        QVERIFY2(message.contains("password"), qPrintable(message));
        QVERIFY(!QFile::exists(rulesPath()));
    }

    void noPkexec()
    {
        qputenv("PATH", dir_.filePath("nothing").toLocal8Bit());
        LH_UdevRules rules;
        QString message;
        QVERIFY(!run(rules, true, &message));
        qputenv("PATH", dir_.filePath("bin").toLocal8Bit() + ":" + old_path_);
        QVERIFY2(message.contains("pkexec was not found"), qPrintable(message));
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int status = 0;
    {
        TestHid test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestUdevRules test;
        status |= QTest::qExec(&test, argc, argv);
    }
    return status;
}
#include "tst_hid.moc"
