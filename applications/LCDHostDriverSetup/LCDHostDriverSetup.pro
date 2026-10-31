# Windows only: installs the WinUSB driver of the G19 display interface.
# LIBWDI_DIR (qmake or environment variable) points to a libwdi source tree
# built with its Visual Studio solution for x64, Release (see README.md).
TARGET = LCDHostDriverSetup
TEMPLATE = app
CONFIG -= qt app_bundle debug_and_release debug
CONFIG += console release

SOURCES += main.c

LIBWDI = $$LIBWDI_DIR
isEmpty(LIBWDI): LIBWDI = $$(LIBWDI_DIR)
INCLUDEPATH += $$quote($$LIBWDI/libwdi)
LIBS += $$quote(-L$$LIBWDI/x64/Release/lib) -llibwdi
LIBS += -lsetupapi -lnewdev -lntdll -lcrypt32 -ladvapi32 -lshell32 -lole32 -luser32 -lgdi32

# libwdi.lib is built against the static C runtime.
QMAKE_CFLAGS_RELEASE ~= s/-MD/-MT/
QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO ~= s/-MD/-MT/
QMAKE_LFLAGS += \"/MANIFESTUAC:level=\'requireAdministrator\' uiAccess=\'false\'\"

DESTDIR = $$lh_destdir()
