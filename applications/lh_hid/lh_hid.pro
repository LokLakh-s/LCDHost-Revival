TARGET = lh_hid
TEMPLATE = lib
CONFIG += shared lh_shared lh_hidapi
QT = core
DEFINES += LH_HID_LIBRARY

LH_SHARED_HEADERS += LH_HidDevice.h LH_HidReportInfo.h LH_UdevRules.h

HEADERS += \
    LH_HidWorker.h \
    LH_HidDevice.h \
    LH_HidReportInfo.h \
    LH_HidRequest.h \
    LH_HidPublisher.h \
    LH_HidThread.h \
    LH_UdevRules.h

SOURCES += \
    LH_HidWorker.cpp \
    LH_HidDevice.cpp \
    LH_HidReportInfo.cpp \
    LH_HidRequest.cpp \
    LH_HidPublisher.cpp \
    LH_HidThread.cpp \
    LH_UdevRules.cpp

# The udev rules, for LH_UdevRules.
RESOURCES += lh_hid.qrc

DESTDIR = $$lh_destdir()
