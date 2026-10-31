# Drives the LCDs through Logitech Gaming Software, with the proprietary
# Logitech LCD SDK, which is not part of this repository. LGLCD_SDK_DIR (qmake
# or environment variable) points to a folder with the files of your copy of
# the SDK: lglcd.h and lglcd.lib for your architecture on Windows, lgLcd.h,
# lgLcdError.h and liblgLcd.a on macOS. See README.md.
TARGET = LH_LgLcdMan
TEMPLATE = lib
DEFINES += LGLCDMAN_LIBRARY
CONFIG += device wow64

include(../Plugins.pri)

SOURCES += \
    $$PLUGIN_SOURCES \
    LH_LgLcdMan.cpp \
    LogitechDevice.cpp \
    LH_LgLcdLegacyThread.cpp \
    LH_LgLcdCallbackThread.cpp \
    LH_LgLcdThread.cpp

HEADERS += \
    $$PLUGIN_HEADERS \
    LH_LgLcdMan.h \
    LogitechDevice.h \
    EventLgLcdNotification.h \
    LH_LgLcdLegacyThread.h \
    EventLgLcdButton.h \
    LH_LgLcdCallbackThread.h \
    LH_LgLcdThread.h

LGLCD_SDK = $$LGLCD_SDK_DIR
isEmpty(LGLCD_SDK): LGLCD_SDK = $$(LGLCD_SDK_DIR)
isEmpty(LGLCD_SDK): error("LH_LgLcdMan needs LGLCD_SDK_DIR, the folder of your Logitech LCD SDK files")
INCLUDEPATH += $$quote($$LGLCD_SDK)

win32 {
    !exists($$LGLCD_SDK/lglcd.h): error("lglcd.h not found in LGLCD_SDK_DIR ($$LGLCD_SDK)")
    LIBS += $$quote($$LGLCD_SDK/lglcd.lib) -ladvapi32
}
macx {
    !exists($$LGLCD_SDK/lgLcd.h): error("lgLcd.h not found in LGLCD_SDK_DIR ($$LGLCD_SDK)")
    LIBS += $$quote($$LGLCD_SDK/liblgLcd.a) -framework CoreFoundation
}

RESOURCES += LgLcdManResources.qrc
