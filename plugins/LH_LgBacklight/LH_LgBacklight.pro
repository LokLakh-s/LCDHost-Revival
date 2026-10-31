TARGET = LH_LgBacklight
TEMPLATE = lib
CONFIG += lh_hid
DEFINES += LGBACKLIGHT_LIBRARY

include(../Plugins.pri)
INCLUDEPATH += ../LgCommon

SOURCES += \
    $$PLUGIN_SOURCES \
    ../LgCommon/LgUdevSetup.cpp \
    LH_LgBacklight.cpp \
    LgBacklightDevice.cpp

HEADERS += \
    $$PLUGIN_HEADERS \
    ../LgCommon/LgUdevSetup.h \
    LH_LgBacklight.h\
    LgBacklightDevice.h

RESOURCES += \
    resources.qrc
