/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QPluginLoader>
#include <QObject>
#include <QSettings>
#include <cstdlib>

#include "launcherapp.h"
#include "launcherwindow.h"
#include "forwardingdelegate.h"
#include "meegoqmllauncher.h"

#include "launcheratoms.h" // contains X11 headers, MUST come last

QTM_USE_NAMESPACE

namespace {
    bool fullscreen = false;
    bool opengl = false;
    bool noRaise = false;
    int width = 1280;
    int height = 800;

    LauncherApp *launcherApp = 0;
    LauncherWindow *launcherWindow = 0;

    // MaxArgCount sets the upper limit on how many of the real
    // command line arguments can be seen with
    // QApplication::arguments()
    const int MaxArgCount = 64;
    int fakeArgc = 0;
    char **fakeArgv = 0;
}

const char * MeeGoQMLLauncher::preinitialisedAppName = "preinit-meego-qml-launcher";

void MeeGoQMLLauncher::prepareForLaunch()
{
    const char * emptyString = "";

    fakeArgc = MaxArgCount;
    fakeArgv = new char* [fakeArgc];

    fakeArgv[0] = const_cast<char*>(MeeGoQMLLauncher::preinitialisedAppName);
    for (int i = 1; i < MaxArgCount; i++)
    {
        fakeArgv[i] = const_cast<char*>(emptyString);
    }

    // Fix for BMC #17521
    XInitThreads();

    // we never, ever want to be saddled with 'native' graphicssystem, as it is
    // amazingly slow. set us to 'raster'. this won't impact GL mode, as we are
    // explicitly using a QGLWidget viewport in those cases.
    QApplication::setGraphicsSystem("raster");

    launcherApp = new LauncherApp(fakeArgc, fakeArgv);
    launcherApp->setApplicationName(MeeGoQMLLauncher::preinitialisedAppName);

    // Set up X stuff
    initAtoms();

    launcherWindow = new LauncherWindow(fullscreen, width, height, opengl);

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
}

int MeeGoQMLLauncher::launch(int argc, char **argv)
{
    QString cmd;
    QString cdata;
    QString app;

    if (!fakeArgv) {
        // Common initializations have not been made yet.
        // Do them now.
        MeeGoQMLLauncher::prepareForLaunch();
    }

    // fix QCoreApplication::arguments() output
    fakeArgc = argc < MaxArgCount ? argc : MaxArgCount;
    for (int i=1; i<fakeArgc; i++) fakeArgv[i] = argv[i];

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
        }
        else if (s == "--width")
        {
            width = atoi (argv[++i]);
        }
        else if (s == "--height")
        {
          height = atoi (argv[++i]);
        }
    }

    launcherApp->setApplicationName(app);
    launcherApp->dbusInit(argc, argv);

    launcherWindow->init(fullscreen, width, height, opengl);
    if (!noRaise)
    {
        qDebug("Raising window");
        launcherWindow->show();
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

        new ForwardingDelegate(list, launcherWindow, launcherApp);
    }

    // ??? now that app name is known, could there be more plugin
    // dirs to search for plugins? prepareForLaunch has already
    // gone through a bunch of dirs.

    return launcherApp->exec();
}
