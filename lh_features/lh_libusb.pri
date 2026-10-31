LH_FEATURES += lh_libusb
# Upstream libusb 1.0. Linux and macOS: found with pkg-config ("libusb-1.0").
# Windows: LIBUSB_DIR (qmake or environment variable) points to an extracted
# official binary release (https://github.com/libusb/libusb/releases); its
# DLL is copied next to the executables.
defineTest(lh_libusb) {
    win32 {
        dir = $$LIBUSB_DIR
        isEmpty(dir): dir = $$(LIBUSB_DIR)
        !exists($$dir/include/libusb.h): \
            error("$$_PRO_FILE_: set LIBUSB_DIR to an extracted libusb binary release, see README.md")
        libdir = $$dir/VS2022/MS64/dll
        INCLUDEPATH *= $$quote($$dir/include)
        LIBS *= $$quote(-L$$libdir) -llibusb-1.0
        lh_mkdir($$lh_systempath($$LH_DIR_BINARIES))
        lh_copy($$lh_systempath($$libdir/libusb-1.0.dll), $$lh_systempath($$LH_DIR_BINARIES))
        export(INCLUDEPATH)
        export(LIBS)
    } else {
        CONFIG *= link_pkgconfig
        PKGCONFIG *= libusb-1.0
        export(CONFIG)
        export(PKGCONFIG)
    }
    return(true)
}
