TEMPLATE=subdirs
exists(../.qmake.cache.in) {
    LH_VERSION=0.2.0
    LH_DIR_SRCROOT=$$quote($$dirname(PWD))
    LH_DIR_DSTROOT=$$quote($$dirname(OUT_PWD))
    QMAKE_SUBSTITUTES=../.qmake.cache.in
}
SUBDIRS=
