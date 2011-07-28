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

#include "meegoqmllauncher.h"
#include <meegolocale.h>

#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>

#define DEBUG_INFO_PATH "/var/tmp/debug.info"

Q_DECLARE_METATYPE(QGraphicsObject *);

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

LauncherWindow::LauncherWindow(bool fullscreen, int width, int height, bool opengl, bool doSetSource, QWidget *parent) :
    QDeclarativeView(parent),
    m_inhibitScreenSaver(false),
    m_useOpenGl(opengl),
    m_usingGl(false),
    locale(new meego::Locale(this))
{
    LauncherApp *app = static_cast<LauncherApp *>(qApp);

    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);

    engine()->setNetworkAccessManagerFactory(new NetworkAccessManagerFactory);
    connect(engine(), SIGNAL(quit()), qApp, SLOT(closeAllWindows()));

    connect((const QObject*)qApp->inputContext(), SIGNAL(inputMethodAreaChanged(QRect)),
            this, SLOT(handleInputMethodAreaChanged(QRect)));
    connect(qApp, SIGNAL(dismissKeyboard()), this, SLOT(dismissKeyboard()));


    QDeclarativeContext *context = rootContext();

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

    loadCommonTranslators();

    // Qt will search each translator for a string translation, starting with
    // the last translator installed working back to the first translator.
    // The first translation found wins.
    app->installTranslator(&qtTranslator);     // General Qt translations
    app->installTranslator(&componentsTranslator); // Common Components translations
    app->installTranslator(&labsTranslator); // MeeGo Labs Components translations
    app->installTranslator(&mediaTranslator);  // Common Media translations

    setEnableDebugInfo(true);
    connect(&m_debugInfoFileWatcher, SIGNAL(fileChanged(QString)), SLOT(debugFileChanged(QString)));
    connect(&m_debugInfoFileWatcher, SIGNAL(directoryChanged(QString)), SLOT(debugDirChanged(QString)));

    if (!app->applicationName().startsWith(MeeGoQMLLauncher::preinitialisedAppNamePrefix))
    {
        init(fullscreen, width, height, opengl, doSetSource);
    }
    else
    {
        // When launching via booster, we always operate at fullscreen
        setWindowFlags(Qt::FramelessWindowHint);
        context->setContextProperty("screenWidth",
                                    qApp->desktop()->rect().width());
        context->setContextProperty("screenHeight",
                                    qApp->desktop()->rect().height());

        setSource(QUrl::fromLocalFile("/usr/share/meego-qml-launcher/common-imports.qml"));
    }

    MGConfItem appSourceDelayItem("/meego/ux/AppSourceDelay");
    if (appSourceDelayItem.value().isValid())
    {
        m_appSourceDelay = appSourceDelayItem.value().toInt();
    }
    else
    {
        // Sane default delay to postpone loading application content
        // until the window has finished animating into the scene
        m_appSourceDelay = 200;
    }
}

LauncherWindow::~LauncherWindow()
{
}

// Contains parts of the initialization that require correct
// application name and parameters which can be set on the command
// line. Application name independent initializations are done in the
// constructor.
void LauncherWindow::init(bool fullscreen, int width, int height,
                          bool opengl, bool doSetSource)
{
    m_useOpenGl = opengl;

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

    QDeclarativeContext *context = rootContext();
    context->setContextProperty("screenWidth", screenWidth);
    context->setContextProperty("screenHeight", screenHeight);

    if (doSetSource) {
        sharePath = QString("/usr/share/") + app->getApplicationPath() + "/main.qml";
        if (!QFile::exists(sharePath))
        {
            qFatal("%s does not exist!", sharePath.toUtf8().data());
        }
    }

    loadAppTranslators();
    connect(locale, SIGNAL(localeChanged()), this, SLOT(loadAppTranslators()));
    app->installTranslator(&appTranslator);    // App specific translations

    connect(locale, SIGNAL(localeChanged()), this, SLOT(localeChanged()));

    if (!app->enableRenderingSwap())
    {
        switchToGLRendering();
    }

    setGeometry(QRect(0, 0, screenWidth, screenHeight));

    connect(app, SIGNAL(foregroundWindowChanged()), this, SLOT(updateOrientationSensorOn()));
}

void LauncherWindow::keyPressEvent ( QKeyEvent * event )
{
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_R)
    {
        QGraphicsObject* window = rootObject();
        if (window)
        {
            QGraphicsObject *appItem = window->property("appItem").value<QGraphicsObject *>();
            if (appItem)
            {
                QVariant orientation = appItem->property("orientation");
                if(orientation.isValid())
                {
                    int orient = orientation.toInt();
                    orient = ((orient + 1) % 4);
                    orientation.setValue(orient);
                    appItem->setProperty("orientation", orientation);
                    return;
                }
            }
        }
    }
    else if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_D)
    {
        setEnableDebugInfo(!m_debugInfoEnabled);
        return;
    }

    QDeclarativeView::keyPressEvent(event);
}

void LauncherWindow::loadCommonTranslators()
{
    qtTranslator.load("qt_" + locale->locale() + ".qm",
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    labsTranslator.load("meegolabs-ux-components_" + locale->locale() + ".qm",
                        QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    componentsTranslator.load("meego-ux-components_" + locale->locale() + ".qm",
                              QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    mediaTranslator.load("meego-ux-media-qml_" + locale->locale() + ".qm",
                         QLibraryInfo::location(QLibraryInfo::TranslationsPath));
}

void LauncherWindow::loadAppTranslators()
{
    LauncherApp *app = static_cast<LauncherApp *>(qApp);

    appTranslator.load(app->getApplicationPath() + "_" + locale->locale() + ".qm",
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
    if(scene() && scene()->focusItem())
    {
        if (QMetaObject::invokeMethod(qApp->inputContext(), "hideOnFocusOut"))
        {
            scene()->focusItem()->clearFocus();
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

        // When an app is launched without the --noraise argument,
        // then we delay loading the application content till after
        // the we window has been mapped to ensure a quick show of
        // the initial splash screen.
        if (m_appSource.isEmpty())
        {
            // Even though we just got mapped, allow the compositor time
            // to animate the window into the sceen before loading content
            QTimer::singleShot(m_appSourceDelay, this, SLOT(loadApplicationContent()));
        }
    }
    return QDeclarativeView::event(event);
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

void LauncherWindow::updateOrientationSensorOn()
{
    LauncherApp *app = static_cast<LauncherApp *>(qApp);
    app->setOrientationSensorOn(app->getForegroundWindow() == winId());
}

void LauncherWindow::switchToGLRendering()
{
    if (m_usingGl || !m_useOpenGl)
        return;

    //go once around event loop to avoid crash in egl
    QTimer::singleShot(1, this, SLOT(doSwitchToGLRendering()));
}

void LauncherWindow::switchToSoftwareRendering()
{
    // no need to change viewport unnecessarily
    if (!m_usingGl)
        return;

    setViewport(0);

    // each time we create a new viewport widget, we must redo our optimisations
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    m_usingGl = false;
}

void LauncherWindow::doSwitchToGLRendering()
{
    QGLFormat format = QGLFormat::defaultFormat();
    format.setSampleBuffers(false);
    setViewport(new QGLWidget(format));

    // each time we create a new viewport widget, we must redo our optimisations
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    m_usingGl = true;
}

void LauncherWindow::localeChanged()
{
    LauncherApp *app = qobject_cast<LauncherApp *>(qApp);

    if(app->getForegroundWindow() != this->winId())
    {
        // We want to have all apps properly saving state so that we can
        // quit the app and then when the app is restored it will startup
        // with the correct locale.
        //
        // But... we need to finish hooking up the bits such that we have
        // a way of knowing if it's safe to exit instead of adding a stress
        // test for all apps.  When this capability lands then replace this
        // block of code with whatever the mechanism is.
        //  
        // app->quit();
        // return;
    }

    loadCommonTranslators();
    loadAppTranslators();
    retranslateUi();
}

void LauncherWindow::debugDirChanged(const QString)
{
    if (QFile::exists(DEBUG_INFO_PATH))
    {
        m_debugInfoFileWatcher.removePath("/var/tmp");
        m_debugInfoFileWatcher.addPath(DEBUG_INFO_PATH);
        debugFileChanged(DEBUG_INFO_PATH);
    }
}

void LauncherWindow::debugFileChanged(const QString)
{
    if (QFile::exists(DEBUG_INFO_PATH))
    {
        QFile data(DEBUG_INFO_PATH);
        if (data.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            m_debugInfo = data.readAll();
        }
        emit debugInfoChanged();
    }
    else
    {
        m_debugInfoFileWatcher.removePath(DEBUG_INFO_PATH);
        m_debugInfoFileWatcher.addPath("/var/tmp");
        m_debugInfo.clear();
        emit debugInfoChanged();
    }
}

void LauncherWindow::setEnableDebugInfo(bool enable)
{
    m_debugInfoEnabled = enable;
    if (m_debugInfoEnabled)
    {
        if (QFile::exists("/var/tmp/debug.info"))
        {
            m_debugInfoFileWatcher.addPath(DEBUG_INFO_PATH);
            debugFileChanged(DEBUG_INFO_PATH);
        }
        else
        {
            m_debugInfoFileWatcher.addPath("/var/tmp");
        }
    }
    else
    {
        m_debugInfoFileWatcher.removePath("/var/tmp");
        m_debugInfoFileWatcher.removePath(DEBUG_INFO_PATH);
        m_debugInfo.clear();
        emit debugInfoChanged();
    }
}
