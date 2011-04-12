include(../config.pri)

QT += declarative opengl network dbus
CONFIG += mobility link_pkgconfig
PKGCONFIG += gconf-2.0 mlite QtOpenGL
MOBILITY += sensors
VERSION = $$LAUNCHER_VERSION
TARGET = meegoqmllauncher
TEMPLATE = lib

SOURCES += launcherwindow.cpp \
    launcherapp.cpp \
    launcheratoms.cpp \
    appproxy.cpp \
    appadaptor.cpp 

HEADERSINSTALL = \
    launcherwindow.h \
    launcherapp.h \
    launcheratoms.h

HEADERS += $$HEADERSINSTALL \
    forwardingdelegate.h \
    appproxy.h \
    appadaptor.h 

OBJECTS_DIR = .obj
MOC_DIR = .moc

target.path += $$LAUNCHER_INSTALL_LIBS

headers.files = $$HEADERSINSTALL
headers.path = $$LAUNCHER_INSTALL_HEADERS/meegoqmllauncher

CONFIG += create_pc create_prl
QMAKE_PKGCONFIG_DESCRIPTION = MeeGo QML Launcher
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += target \
    headers
