/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QPluginLoader>
#include <QObject>
#include <cstdlib>

#include "launcherapp.h"
#include "launcherwindow.h"
#include "forwardingdelegate.h"
#include "launcheratoms.h" // contains X11 headers, MUST come last

QTM_USE_NAMESPACE

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

    // Fix for BMC #17521
    XInitThreads();

    // we never, ever want to be saddled with 'native' graphicssystem, as it is
    // amazingly slow. set us to 'raster'. this won't impact GL mode, as we are
    // explicitly using a QGLWidget viewport in those cases.
    QApplication::setGraphicsSystem("raster");

    // Set up application
    LauncherApp a(argc, argv);
    a.setApplicationName(app);
    a.dbusInit(argc, argv);

    // Set up X stuff
    initAtoms();

    // Create window
    LauncherWindow *window = new LauncherWindow(fullscreen, width, height, opengl);
    if (!noRaise)
    {
        qDebug("Raising window");
        window->show();
    }
    else
    {
        qDebug("Not raising window");
    }

    if (!cmd.isEmpty() || !cdata.isEmpty())
    {
        QStringList list;
        list << cmd;
        list << cdata;

        new ForwardingDelegate(list, window, &a);
    }

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
