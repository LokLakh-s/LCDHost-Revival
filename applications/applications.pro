TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = lh_hid LCDHost

# The G19 driver installer needs libwdi, see README.md.
win32 {
    LIBWDI = $$LIBWDI_DIR
    isEmpty(LIBWDI): LIBWDI = $$(LIBWDI_DIR)
    !isEmpty(LIBWDI): SUBDIRS += LCDHostDriverSetup
    else: message("LIBWDI_DIR is not set: LCDHostDriverSetup (G19 driver installer) is not built")
}
