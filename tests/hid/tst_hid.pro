# Unit tests of the HID code that needs no device, and of the udev rules
# installer (with fake pkexec and udevadm commands). Build and run them with:
#   qmake tests/hid && make && ./tst_hid
# (from a build directory, giving the path to this file to qmake).
TEMPLATE = app
TARGET = tst_hid
QT = core testlib
CONFIG += console testcase
CONFIG -= app_bundle
DEFINES += LH_HID_LIBRARY

SRC = $$PWD/../..
INCLUDEPATH += \
    $$SRC/applications/lh_hid \
    $$SRC/plugins/LH_Lg160x43 \
    $$SRC/plugins/LH_Lg320x240

SOURCES += \
    tst_hid.cpp \
    $$SRC/applications/lh_hid/LH_HidReportInfo.cpp \
    $$SRC/applications/lh_hid/LH_UdevRules.cpp \
    $$SRC/plugins/LH_Lg160x43/Lg160x43Model.cpp \
    $$SRC/plugins/LH_Lg320x240/G19Keys.cpp

HEADERS += $$SRC/applications/lh_hid/LH_UdevRules.h
RESOURCES += $$SRC/applications/lh_hid/lh_hid.qrc
