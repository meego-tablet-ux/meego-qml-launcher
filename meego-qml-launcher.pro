include(config.pri)

CONFIG += ordered
TARGET = meego-qml-launcher
TEMPLATE = subdirs
SUBDIRS = src launcher

VERSION = $$LAUNCHER_VERSION
PROJECT_NAME = $$TARGET

dist.commands += rm -fR $${PROJECT_NAME}-$${VERSION} &&
dist.commands += git clone . $${PROJECT_NAME}-$${VERSION} &&
dist.commands += rm -fR $${PROJECT_NAME}-$${VERSION}/.git &&
dist.commands += tar jcpvf $${PROJECT_NAME}-$${VERSION}.tar.bz2 $${PROJECT_NAME}-$${VERSION}
QMAKE_EXTRA_TARGETS += dist
