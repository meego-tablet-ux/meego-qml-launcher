/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QGraphicsItem>
#include <QDeclarativeContext>
#include <QDeclarativeNetworkAccessManagerFactory>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGLFormat>
#include <QLibraryInfo>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkProxy>
#include <QResizeEvent>
#include <QSettings>
#include <QTimer>
#include <QTextStream>
#include <QTranslator>
#include <QVariant>
#include <QX11Info>
#include <MGConfItem>
#include <QInputContext>

#include "launcherwindow.h"
#include "launcheratoms.h"
#include "launcherapp.h"

#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>

class NetworkAccessManagerFactory : public QDeclarativeNetworkAccessManagerFactory
{
public:
    virtual QNetworkAccessManager *create(QObject *parent);
};

QNetworkAccessManager *NetworkAccessManagerFactory::create(QObject *parent)
{
    QUrl httpProxy(getenv("http_proxy"));
    QNetworkAccessManager *nam = new QNetworkAccessManager(parent);
    if (!httpProxy.isEmpty())
    {
        QNetworkProxy proxy(QNetworkProxy::HttpCachingProxy,
                            httpProxy.host(),
                            httpProxy.port());
        nam->setProxy(proxy);
    }

    QNetworkDiskCache *cache = new QNetworkDiskCache();
    cache->setCacheDirectory(QDir::homePath() + "/.cache/" + qApp->applicationName());
    nam->setCache(cache);

    return nam;
}

LauncherWindow::LauncherWindow(bool fullscreen, int width, int height, bool opengl, bool setSource, QWidget *parent) :
    QWidget(parent),
    m_inhibitScreenSaver(false)
{
    LauncherApp *app = static_cast<LauncherApp *>(qApp);

    setWindowTitle(app->applicationName());
    setWindowIconText(app->applicationName());

    int screenWidth;
    int screenHeight;
    if (fullscreen)
    {
        screenWidth = qApp->desktop()->rect().width();
        screenHeight = qApp->desktop()->rect().height();
        setWindowFlags(Qt::FramelessWindowHint);
    }
    else
    {
        screenWidth = width;
        screenHeight = height;
    }

    view = new QDeclarativeView(this);
    view->engine()->setNetworkAccessManagerFactory(new NetworkAccessManagerFactory);
    connect(view->engine(), SIGNAL(quit()), qApp, SLOT(closeAllWindows()));
    connect((const QObject*)qApp->inputContext(), SIGNAL(inputMethodAreaChanged(QRect)),
            this, SLOT(handleInputMethodAreaChanged(QRect)));
    connect(qApp, SIGNAL(dismissKeyboard()), this, SLOT(dismissKeyboard()));

    QDeclarativeContext *context = view->rootContext();
    context->setContextProperty("screenWidth", screenWidth);
    context->setContextProperty("screenHeight", screenHeight);
    context->setContextProperty("qApp", qApp);
    context->setContextProperty("mainWindow", this);
    context->setContextProperty("theme_name", MGConfItem("/meego/ux/theme").value().toString());
    foreach (QString key, app->themeConfig->allKeys())
    {
        if (key.contains("Size") || key.contains("Padding") ||
            key.contains("Width") ||key.contains("Height") ||
            key.contains("Margin") || key.contains("Thickness"))
        {
            context->setContextProperty("theme_" + key, app->themeConfig->value(key).toInt());
        }
        else if (key.contains("Opacity"))
        {
            context->setContextProperty("theme_" + key, app->themeConfig->value(key).toDouble());
        }
        else
        {
            context->setContextProperty("theme_" + key, app->themeConfig->value(key));
        }
    }

    if (setSource) {
      sharePath = QString("/usr/share/") + app->applicationName() + "/";
      if (!QFile::exists(sharePath + "main.qml"))
      {
        qFatal("%s does not exist!", sharePath.toUtf8().data());
      }
    }

    loadTranslators();
    connect(app, SIGNAL(localeSettingsChanged()), this, SLOT(loadTranslators()));

    // Qt will search each translator for a string translation, starting with
    // the last translator installed working back to the first translator.
    // The first translation found wins.
    app->installTranslator(&qtTranslator);     // General Qt translations
    app->installTranslator(&commonTranslator); // Common Components translations
    app->installTranslator(&mediaTranslator);  // Common Media translations
    app->installTranslator(&appTranslator);    // App specific translations

    if (setSource)
    {
      view->setSource(QUrl(sharePath + "main.qml"));
    }

    if (opengl)
    {
        QGLFormat format = QGLFormat::defaultFormat();
        format.setSampleBuffers(false);
        view->setViewport(new QGLWidget(format));
    }

    setGeometry(QRect(0, 0, screenWidth, screenHeight));
}

LauncherWindow::~LauncherWindow()
{
}

void LauncherWindow::loadTranslators()
{
    LauncherApp *app = static_cast<LauncherApp *>(qApp);

    qtTranslator.load("qt_" + QLocale::system().name() + ".qm",
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    commonTranslator.load("meegolabs-ux-components_" + QLocale::system().name() + ".qm",
                          QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    mediaTranslator.load("meego-ux-media-qml_" + QLocale::system().name() + ".qm",
                         QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    appTranslator.load(app->applicationName() + "_" + QLocale::system().name() + ".qm",
                       QLibraryInfo::location(QLibraryInfo::TranslationsPath));
}

void LauncherWindow::triggerSystemUIMenu()
{
    QDBusInterface iface("com.nokia.systemui","/statusindicatormenu","com.meego.core.MStatusIndicatorMenu");
    if(iface.isValid()) iface.call("open");
}

void LauncherWindow::goHome()
{
    setWindowState(windowState() ^ Qt::WindowMinimized);
}

// Called by LauncherApplication when it wants to dismiss the
// keyboard.  hideOnFocusOut() is a slot on MInputContext, that seems
// to be required, it doesn't get it right if you just drop the focus
// (bug in MInputContext?)
void LauncherWindow::dismissKeyboard()
{
    if(view->scene() && view->scene()->focusItem())
    {
        if (QMetaObject::invokeMethod(qApp->inputContext(), "hideOnFocusOut"))
        {
            view->scene()->focusItem()->clearFocus();
        }
    }
}

void LauncherWindow::forwardCall(const QStringList& parameters)
{
    m_call = parameters;
    emit call(parameters);
    emit callChanged();
}

void LauncherWindow::setActualOrientation(int orientation)
{
    m_actualOrientation = orientation;

    if (!winId())
    {
        return;
    }

    Atom orientationAtom = XInternAtom(QX11Info::display(), "_MEEGO_ORIENTATION", false);
    XChangeProperty(QX11Info::display(), winId(), orientationAtom, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char*)&m_actualOrientation, 1);
}

bool LauncherWindow::event (QEvent * event)
{
    if (event->type() == QEvent::Show)
    {
        setActualOrientation(m_actualOrientation);
    }
    return QWidget::event(event);
}

void LauncherWindow::setInhibitScreenSaver(bool inhibit)
{
    m_inhibitScreenSaver = inhibit;

    Atom inhibitAtom = XInternAtom(QX11Info::display(), "_MEEGO_INHIBIT_SCREENSAVER", false);
    if (inhibit)
    {
        XChangeProperty(QX11Info::display(), winId(), inhibitAtom, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char*)&m_inhibitScreenSaver, 1);
    }
    else
    {
        XDeleteProperty(QX11Info::display(), winId(), inhibitAtom);
    }
}
