TARGET = meego-qml-launcher
TEMPLATE = app
CONFIG += mobility link_pkgconfig
MOBILITY += sensors
PKGCONFIG += gconf-2.0 mlite

INCLUDEPATH += . ../src
LIBS += ../src/libmeegoqmllauncher.so

SOURCES += main.cpp

target.files += $$TARGET
target.path += $$INSTALL_ROOT/usr/bin
INSTALLS += target

