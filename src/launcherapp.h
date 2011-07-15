/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef LAUNCHER_APP_H
#define LAUNCHER_APP_H

#include <QApplication>
#include <QTimer>
#include <QOrientationSensor>

QTM_USE_NAMESPACE

class QSettings;

class LauncherApp : public QApplication
{
    Q_OBJECT
    Q_PROPERTY(int orientation READ getOrientation NOTIFY orientationChanged);
    Q_PROPERTY(bool orientationLocked READ getOrientationLocked WRITE setOrientationLocked);
    Q_PROPERTY(int foregroundWindow READ getForegroundWindow NOTIFY foregroundWindowChanged);
    Q_PROPERTY(bool preinit READ getPreinit NOTIFY preinitChanged);
    Q_PROPERTY(QString splashImage READ getSplashImage NOTIFY splashImageChanged);
    Q_PROPERTY(bool restoreRequested READ getRestoreRequested);
public:
    explicit LauncherApp(int &argc, char **argv);
    void dbusInit(int argc, char** argv);

    int getOrientation() {
        return orientation;
    }
    void setOrientation(int o) {
        orientation = o;
        emit orientationChanged();
    }

    bool getOrientationLocked() {
        return orientationLocked;
    }
    void setOrientationLocked(bool locked);

    int getForegroundWindow() {
        return foregroundWindow;
    }

    bool getPreinit() {
        return preinit;
    }

    void setPreinit(const bool _preinit);

    QSettings *themeConfig;

    QString getSplashImage();
    void updateSplash() {
        emit splashImageChanged();
    }

    bool enableRenderingSwap() {
        return m_enableRenderingSwap;
    }

    void setRestoreRequested(bool requested) {
        m_restoreRequested = requested;
    }
    bool getRestoreRequested() {
        return m_restoreRequested;
    }

public slots:
    void appPageLoaded();
    void raise(const QStringList& args);
    void hide();
    void keyboardActive();
    void keyboardTimeout();
    void setOrientationSensorOn(bool value);
    void launchDesktopByName(QString name, QString cmd = QString(), QString cdata = QString());

signals:
    void orientationChanged();
    void foregroundWindowChanged();
    void dismissKeyboard();
    void preinitChanged();
    void splashImageChanged();

protected:
    virtual bool x11EventFilter(XEvent *event);

private slots:
    void onOrientationChanged();

private:
    bool isSystemModelDialog(unsigned target);

    int orientation;
    bool orientationLocked;
    bool noRaise;
    int foregroundWindow;
    bool preinit;

    int xinputOpcode; // XInput2 extension identifier

    unsigned long lastButtonTime;
    int lastButtonX;
    int lastButtonY;
    QTimer keyboardTimer;
    bool keyboardIsActive;
    QOrientationSensor orientationSensor;

    bool m_enableRenderingSwap;
    bool m_restoreRequested;
};

#endif // LAUNCHER_APP_H
