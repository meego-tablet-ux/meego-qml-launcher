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

class QSettings;

class LauncherApp : public QApplication
{
    Q_OBJECT
    Q_PROPERTY(int orientation READ getOrientation NOTIFY orientationChanged)
    Q_PROPERTY(int foregroundWindow READ getForegroundWindow NOTIFY foregroundWindowChanged);

public:
    explicit LauncherApp(int &argc, char **argv, const QString &appIdentifier, bool noRaise);

    int getOrientation() {
        return orientation;
    }
    void setOrientation(int o) {
        orientation = o;
        emit orientationChanged();
    }

    int getForegroundWindow() {
        return foregroundWindow;
    }

    QSettings *themeConfig;

public slots:
    void appPageLoaded();
    void raise(const QStringList& args);
    void keyboardActive();
    void keyboardTimeout();

signals:
    void orientationChanged();
    void foregroundWindowChanged();
    void dismissKeyboard();

protected:
    virtual bool x11EventFilter(XEvent *event);

private:
    void dbusInit(int argc, char** argv);

    int orientation;
    bool noRaise;
    int foregroundWindow;

    int xinputOpcode; // XInput2 extension identifier

    unsigned long lastButtonTime;
    int lastButtonX;
    int lastButtonY;
    QTimer keyboardTimer;
    bool keyboardIsActive;
};

#endif // LAUNCHER_APP_H
