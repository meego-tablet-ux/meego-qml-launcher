/*
 * This file was generated by qdbusxml2cpp version 0.7
 * Command line was: qdbusxml2cpp -a appadaptor -c MeeGoAppAdaptor com.meego.launcher.xml
 *
 * qdbusxml2cpp is Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef APPADAPTOR_H_1301356598
#define APPADAPTOR_H_1301356598

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;

/*
 * Adaptor class for interface com.meego.launcher
 */
class MeeGoAppAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.meego.launcher")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.meego.launcher\">\n"
"    <method name=\"raise\">\n"
"      <arg type=\"as\" name=\"cmdline\"/>\n"
"    </method>\n"
"    <method name=\"hide\"/>\n"
"  </interface>\n"
        "")
public:
    MeeGoAppAdaptor(QObject *parent);
    virtual ~MeeGoAppAdaptor();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    void hide();
    void raise(const QStringList &cmdline);
Q_SIGNALS: // SIGNALS
};

#endif
