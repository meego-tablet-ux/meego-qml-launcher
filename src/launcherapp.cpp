/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QtCore>
#include <QSettings>
#include <MGConfItem>
#include <stdio.h>
#include <QX11Info>
#include <X11/Xatom.h>
#include <QInputContext>
#include <QInputContextFactory>
#include <cstdlib>

// Mobility
#include <QOrientationReading>
#include <QOrientationSensor>

#include "launcherapp.h"
#include "launcheratoms.h"
#include "launcherwindow.h"
#include "appadaptor.h"
#include "appproxy.h"

#include <X11/extensions/XInput2.h>

// Tap detection: press/release within 150ms and 20 pixels
#define TAP_TIME_MS 150
#define TAP_SIZE 20

// Dismiss the keyboard after 200ms if nothing tries to use it
#define KEYBOARD_DISMISS_TIMEOUT 200

void messageHandler(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s\n", msg);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", msg);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        abort();
    }
}

LauncherApp::LauncherApp(int &argc, char **argv) :
    QApplication(argc, argv),
    orientation(1),
    foregroundWindow(0)
{
    connect(&orientationSensor, SIGNAL(readingChanged()), SLOT(onOrientationChanged()));
    orientationSensor.start();

    QString theme = MGConfItem("/meego/ux/theme").value().toString();
    QString themeFile = QString("/usr/share/themes/") + theme + "/theme.ini";
    if(!QFile::exists(themeFile))
    {
        // fallback
        themeFile = QString("/usr/share/themes/1024-600-10/theme.ini");
    }
    themeConfig = new QSettings(themeFile, QSettings::NativeFormat, this);

    setFont(QFont(themeConfig->value("fontFamily").toString(), themeConfig->value("fontPixelSizeMedium").toInt()));

    QInputContext *ic = QInputContextFactory::create("MInputContext", 0);
    if(ic) {
        setInputContext(ic);
        connect(ic, SIGNAL(keyboardActive()), this, SLOT(keyboardActive()));
    }

    qInstallMsgHandler(messageHandler);

    int dummy;
    if(!XQueryExtension(QX11Info::display(), "XInputExtension",
                        &xinputOpcode, &dummy, &dummy))
        xinputOpcode = -1;

    // Brutal hack: MTF (underneath the input context somewhere)
    // registers a duplicate session on a default name (sucked from
    // QApplication I guess) that collides.  We don't need it, so drop
    // it.
    QDBusConnection::sessionBus().unregisterService("com.nokia.meego-qml-launcher");
}

void LauncherApp::dbusInit(int argc, char** argv)
{
    QString service = "com.meego.launcher." + applicationName();
    QString object = "/com/meego/launcher";

    QDBusConnection bus = QDBusConnection::sessionBus();
    if(!bus.isConnected())
        return;

    // Are we the first app of our kind?
    new MeeGoAppAdaptor(this);
    if(bus.registerService(service) && bus.registerObject(object, this))
        return;

    // Nope: send a "raise" command to whoever owns the object and bail
    QStringList args;
    for(int i=0; i<argc; i++)
        args << argv[i];
    MeeGoAppProxy(service, object, bus).raise(args);

    std::exit(0);
}

void LauncherApp::raise(const QStringList& parameters)
{
    //So we can launch an app without bringing it to the front...
    bool noRaise = parameters.contains("--noraise");

    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        if (!noRaise) {
            widget->show();
            widget->activateWindow();
            widget->raise();
        }

        QStringList args;
        for (int i = 0; i < parameters.length(); i++)
        {
            if (parameters[i] == "--cmd" && i + 1 < parameters.length())
                args << parameters[++i];
            else if (parameters[i] == "--cdata" && i + 1 < parameters.length())
                args << parameters[++i];
        }

        LauncherWindow *w = static_cast<LauncherWindow *>(widget);
        w->forwardCall(args);
    }
}

void LauncherApp::hide()
{
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        widget->hide();
    }
}

void LauncherApp::appPageLoaded()
{
    qint64 time = QDateTime::currentDateTime().toMSecsSinceEpoch();

    QFile file(QDir::homePath() + "/.config/qml-launcher/" + applicationName());
    if(!file.exists()) return;

    if(!file.open(QIODevice::WriteOnly | QIODevice::QIODevice::Append)) return;

    file.write("end: " + QString::number(time).toAscii() +"\n");
    file.close();
}

void LauncherApp::keyboardActive()
{
    keyboardIsActive = true;
}

void LauncherApp::keyboardTimeout()
{
    // If we get here and the keyboard hasn't shown "active", then the
    // user has done a tap (at least, a tap visible to us), and the
    // actions associated with that tap don't involve communication
    // with the VKB server.  We take that to mean that it's an
    // interaction with some *other* part of the interface, which
    // means we should release focus and dismiss the keyboard.
    if(!keyboardIsActive)
        emit dismissKeyboard();
}

bool LauncherApp::x11EventFilter(XEvent *event)
{
    Display *dpy = QX11Info::display();

    // Detect a "tap" anywhere in the app (uses any button ID, not
    // just the first: you can "tap" with a second finger for example,
    // not sure if that's semantically correct or not...) by snooping
    // the events.  Use this to dismiss the virtual keyboard.
    //
    // Note: we have to use XInput2 here, because that's what the Qt
    // is selecting for.  And the interaction is subtle: XInput2
    // requires that there be a XGetEventData() call to retrieve the
    // new-style/big event structure, and while we're at the head of
    // the event handling here, Qt has *already* called this.  So the
    // XGenericEventCookie::data field is already populated.  Seems
    // fragile, but works...
    if(event->type == GenericEvent && event->xcookie.extension == xinputOpcode)
    {
        XIDeviceEvent *xi = (XIDeviceEvent*)event->xcookie.data;
        if(xi->evtype == XI_ButtonPress)
        {
            lastButtonTime = xi->time;
            lastButtonX = xi->root_x;
            lastButtonY = xi->root_y;
            keyboardIsActive = false;
        }
        else if(xi->evtype == XI_ButtonRelease)
        {
            unsigned long dt = xi->time - lastButtonTime;
            int dx = abs(xi->root_x - lastButtonX);
            int dy = abs(xi->root_y - lastButtonY);
            if(dt < TAP_TIME_MS && dx < TAP_SIZE && dy < TAP_SIZE)
            {
                keyboardTimer.singleShot(KEYBOARD_DISMISS_TIMEOUT, this,
                                         SLOT(keyboardTimeout()));
            }
        }
    }

    Atom activeWindowAtom = getAtom(ATOM_NET_ACTIVE_WINDOW);

    // Foreground window detection
    if (event->type == PropertyNotify &&
            event->xproperty.atom == activeWindowAtom)
    {
        Atom actualType;
        int actualFormat;
        unsigned long numWindowItems, bytesLeft;
        unsigned char *data = NULL;

        int result = XGetWindowProperty(dpy,
                                        DefaultRootWindow(dpy),
                                        activeWindowAtom,
                                        0, 0x7fffffff,
                                        false, XA_WINDOW,
                                        &actualType,
                                        &actualFormat,
                                        &numWindowItems,
                                        &bytesLeft,
                                        &data);

        if (result == Success && data != None)
        {
            Window w = *(Window *)data;
            foregroundWindow = (int)w;
            XFree(data);
            emit foregroundWindowChanged();
        }
    }

    return QApplication::x11EventFilter(event);
}

void LauncherApp::setOrientationLocked(bool locked)
{
    orientationLocked = locked;
    if (locked)
    {
        orientationSensor.stop();
        emit stopOrientationSensor();
    }
    else
    {
        orientationSensor.start();
        emit startOrientationSensor();
    }
}

// Copied from libmeegotouch, which we don't link against.  We need it
// defined so we can connect a signal to the MInputContext object
// (loaded from a plugin) that uses this type.
namespace M {
    enum OrientationAngle { Angle0=0, Angle90=90, Angle180=180, Angle270=270 };
}

void LauncherApp::onOrientationChanged()
{
    int orientation = orientationSensor.reading()->orientation();

    qDebug("Handling orientation %d", orientation);
    int qmlOrient;
    M::OrientationAngle mtfOrient;
    switch (orientation)
    {
    case QOrientationReading::LeftUp:
        mtfOrient = M::Angle270;
        qmlOrient = 2;
        break;
    case QOrientationReading::TopDown:
        mtfOrient = M::Angle180;
        qmlOrient = 3;
        break;
    case QOrientationReading::RightUp:
        mtfOrient = M::Angle90;
        qmlOrient = 0;
        break;
    default: // assume QOrientationReading::TopUp
        mtfOrient = M::Angle0;
        qmlOrient = 1;
        break;
    }

    ((LauncherApp*)qApp)->setOrientation(qmlOrient);

    // Need to tell the MInputContext plugin to rotate the VKB too
    QMetaObject::invokeMethod(inputContext(),
                              "notifyOrientationChanged",
                              Q_ARG(M::OrientationAngle, mtfOrient));
}
