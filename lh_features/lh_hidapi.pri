LH_FEATURES += lh_hidapi
# Upstream hidapi (https://github.com/libusb/hidapi). Linux: pkg-config
# "hidapi-hidraw"; macOS: pkg-config "hidapi". Windows: HIDAPI_DIR (qmake or
# environment variable) points to an extracted hidapi-win.zip release; its
# DLL is copied next to the executables.
defineTest(lh_hidapi) {
    win32 {
        dir = $$HIDAPI_DIR
        isEmpty(dir): dir = $$(HIDAPI_DIR)
        !exists($$dir/include/hidapi.h): \
            error("$$_PRO_FILE_: set HIDAPI_DIR to an extracted hidapi-win.zip release, see README.md")
        libdir = $$dir/x64
        INCLUDEPATH *= $$quote($$dir/include)
        LIBS *= $$quote(-L$$libdir) -lhidapi
        lh_mkdir($$lh_systempath($$LH_DIR_BINARIES))
        lh_copy($$lh_systempath($$libdir/hidapi.dll), $$lh_systempath($$LH_DIR_BINARIES))
        export(INCLUDEPATH)
        export(LIBS)
    } else {
        CONFIG *= link_pkgconfig
        macx: PKGCONFIG *= hidapi
        else: PKGCONFIG *= hidapi-hidraw
        export(CONFIG)
        export(PKGCONFIG)
    }
    return(true)
}
