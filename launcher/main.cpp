/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QPluginLoader>
#include <QInputContext>
#include <QObject>
#include <QOrientationReading>
#include <QOrientationSensor>
#include <QOrientationFilter>
#include <cstdlib>

#include "launcherapp.h"
#include "launcheratoms.h"
#include "launcherwindow.h"
#include "forwardingdelegate.h"

QTM_USE_NAMESPACE

// Copied from libmeegotouch, which we don't link against.  We need it
// defined so we can connect a signal to the MInputContext object
// (loaded from a plugin) that uses this type.
namespace M {
    enum OrientationAngle { Angle0=0, Angle90=90, Angle180=180, Angle270=270 };
}

class OrientationSensorFilter : public QOrientationFilter
{
    bool filter(QOrientationReading *reading)
    {
        int qmlOrient;
        M::OrientationAngle mtfOrient;
        switch (reading->orientation())
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
        QMetaObject::invokeMethod(qApp->inputContext(),
                                  "notifyOrientationChanged",
                                  Q_ARG(M::OrientationAngle, mtfOrient));
        return false;
    }
};

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        qWarning("USAGE: %s [options] APPNAME", argv[0]);
        std::exit(-1);
    }

    bool fullscreen = false;
    bool opengl = false;
    bool noRaise = false;
    int width = 1280;
    int height = 800;
    QString cmd;
    QString cdata;
    QString app;
    for (int i=1; i<argc; i++)
    {
        QString s(argv[i]);
        if (s == "--opengl")
        {
            opengl = true;
        }
        else if (s == "--fullscreen")
        {
            fullscreen = true;
        }
        else if (s == "--cmd")
        {
            cmd = QString(argv[++i]);
        }
        else if (s == "--cdata")
        {
            cdata = QString(argv[++i]);
        }
        else if (s == "--app")
        {
            app = QString(argv[++i]);
        }
        else if (s == "--noraise")
        {
            noRaise = true;
        } else if (s == "--width")
        {
            width = atoi (argv[++i]);
        } else if (s == "--height")
        {
          height = atoi (argv[++i]);
        }
    }

    QString identifier = QString(app);
    LauncherApp a(argc, argv, identifier, noRaise);

    initAtoms ();

    LauncherWindow *window = new LauncherWindow(fullscreen, width, height, opengl, noRaise);
    if (!noRaise)
        window->show();

    if (!cmd.isEmpty() || !cdata.isEmpty())
    {
        QStringList list;
        list << cmd;
        list << cdata;

        new ForwardingDelegate(list, window, &a);
    }

    QOrientationSensor sensor;
    OrientationSensorFilter filter;
    sensor.addFilter(&filter);
    sensor.start();

    QObject::connect(&a, SIGNAL(startOrientationSensor()), &sensor, SLOT(start()));
    QObject::connect(&a, SIGNAL(stopOrientationSensor()), &sensor, SLOT(stop()));

    foreach (QString path, QCoreApplication::libraryPaths())
    {
        QPluginLoader loader(path + "/libmultipointtouchplugin.so");
        loader.load();
        if (loader.isLoaded())
        {
            loader.instance();
            break;
        }
    }

    return a.exec();
}
