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
#include <QDir>
#include <cstdlib>
#include <sys/prctl.h>

#include "launcherapp.h"
#include "launcherwindow.h"
#include "forwardingdelegate.h"
#include "meegoqmllauncher.h"

#include "launcheratoms.h" // contains X11 headers, MUST come last

QTM_USE_NAMESPACE

const char * MeeGoQMLLauncher::preinitialisedAppNamePrefix = "preinit-meego-qml-launcher";

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
    static char preinitialisedAppName[sizeof(MeeGoQMLLauncher::preinitialisedAppNamePrefix) + 8];
}

void MeeGoQMLLauncher::prepareForLaunch()
{
    static QString appNameFormat(MeeGoQMLLauncher::preinitialisedAppNamePrefix);    
    appNameFormat += QString("-%1");

    static QByteArray appName;

    // Append pid to appName to make it unique. This is required because the
    // libminputcontext.so instantiates MComponentData, which in turn registers
    // a dbus service with the application's name.
    appName = appNameFormat.arg(getpid()).toLatin1();

    const char * emptyString = "";

    fakeArgc = MaxArgCount;
    fakeArgv = new char* [fakeArgc];

    fakeArgv[0] = const_cast<char*>(appName.constData()) ;
    for (int i = 1; i < MaxArgCount; i++)
    {
        fakeArgv[i] = const_cast<char*>(emptyString);
    }

    // Fix for BMC #17521
    XInitThreads();


    QSettings i18n(QDir::homePath() + "/.config/sysconfig/i18n", QSettings::NativeFormat);

    setenv("LANG", i18n.value("LANG","en_US.UTF-8").toString().toAscii().data(), 1);

    // we never, ever want to be saddled with 'native' graphicssystem, as it is
    // amazingly slow. set us to 'raster'. this won't impact GL mode, as we are
    // explicitly using a QGLWidget viewport in those cases.
    QApplication::setGraphicsSystem("raster");

    launcherApp = new LauncherApp(fakeArgc, fakeArgv);
    launcherApp->setApplicationName(appName);
    launcherApp->setPreinit(true);

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
    QString appName;

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
            // app is to specify application path, and used as application name
            // when "--appname" is not set
            app = QString(argv[++i]);
        }
        else if (s == "--appname")
        {
            appName = QString(argv[++i]);
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

    launcherApp->setRestoreRequested(cmd == "restore");
    // '--appname' is used to allow one binary running as different applications
    // if it is not set, fall back to use application path '--app'
    launcherApp->setApplicationPath(app);
    if (appName.isEmpty()) 
    {
      launcherApp->setApplicationName(app);
    
      // Set process name so all QML apps do not look like the
      // same app for profiling/development tools
      prctl(PR_SET_NAME, app.mid(0, 16).toAscii().data(), 0, 0, 0);
    } 
    else 
    {
      launcherApp->setApplicationName(appName);

      // Set process name so all QML apps do not look like the
      // same app for profiling/development tools
      prctl(PR_SET_NAME, appName.mid(0, 16).toAscii().data(), 0, 0, 0);
    }

    launcherApp->updateSplash();
    launcherApp->dbusInit(argc, argv);
    launcherApp->setPreinit(false);

    launcherWindow->init(fullscreen, width, height, opengl);
    if (!noRaise)
    {
        launcherApp->setOrientationSensorOn(true);
        launcherWindow->show();
    }
    else
    {
        // The idea behind starting an app with --noraise is that
        // the application content is loaded and executing even though
        // the acutal toplevel window is not displayed yet.  Since
        // we normally delay loading the app content till after the
        // initial window showing the splash has been mapped, then we
        // need to explicitly trigger loading the app content in this case
        launcherWindow->loadApplicationContent();
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
