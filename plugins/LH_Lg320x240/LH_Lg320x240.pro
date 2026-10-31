TARGET = LH_Lg320x240
TEMPLATE = lib
DEFINES += LG320x240_LIBRARY
CONFIG += lh_plugin lh_api5plugin lh_libusb lh_hid

SOURCES += \
    G19Keys.cpp \
    LH_Lg320x240.cpp \
    LogitechG19.cpp

HEADERS += \
    G19Keys.h \
    LH_Lg320x240.h \
    LogitechG19.h

# ShellExecuteExW, to start the elevated driver installer.
win32: LIBS *= -lshell32

DESTDIR=$$lh_destdir()
