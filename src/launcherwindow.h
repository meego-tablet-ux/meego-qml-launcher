/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef LAUNCHERWINDOW_H
#define LAUNCHERWINDOW_H

#include <QWidget>
#include <QDeclarativeView>
#include <QTranslator>

class LauncherWindow : public QDeclarativeView
{
    Q_OBJECT
    Q_PROPERTY(int winId READ winId NOTIFY winIdChanged)
    Q_PROPERTY(int actualOrientation  READ actualOrientation WRITE setActualOrientation)
    Q_PROPERTY(bool inhibitScreenSaver READ inhibitScreenSaver WRITE setInhibitScreenSaver)
    Q_PROPERTY(QStringList call READ getCall NOTIFY callChanged)

public:
    LauncherWindow(bool fullscreen, int width, int height, bool opengl, bool doSetSource = true, QWidget *parent = NULL);
    ~LauncherWindow();

    void forwardCall(const QStringList& parameters);

    int actualOrientation() {
        return m_actualOrientation;
    }
    void setActualOrientation(int orientation);

    void switchToGLRendering();
    void switchToSoftwareRendering();

    bool inhibitScreenSaver() {
        return m_inhibitScreenSaver;
    }
    void setInhibitScreenSaver(bool inhibit);

    QStringList getCall() {
        return m_call;
    }

    // temporary method to enable existing clients
    // that still think the LauncherWindow has an
    // extra outer QWidget
    QDeclarativeView* getDeclarativeView() {
        return this;
    }

signals:
    void call(const QStringList& parameters);
    void winIdChanged();
    void vkbHeight(int x, int y, int width, int height);
    void callChanged();

public slots:
    void triggerSystemUIMenu();
    void goHome();
    int winId() const {
        return internalWinId();
    }
    void handleInputMethodAreaChanged(const QRect &newArea) {
        emit vkbHeight(newArea.x(), newArea.y(), newArea.width(), newArea.height());
    }
    void dismissKeyboard();

private slots:
    void loadTranslators();
    void updateOrientationSensorOn();
    void doSwitchToGLRendering();

protected:
    bool event(QEvent * event);
    void keyPressEvent ( QKeyEvent * event );

private:
    QString sharePath;
    QTranslator qtTranslator;
    QTranslator commonTranslator;
    QTranslator mediaTranslator;
    QTranslator appTranslator;
    int m_actualOrientation;
    bool m_inhibitScreenSaver;
    QStringList m_call;
    bool m_useOpenGl;
    bool m_usingGl;
};

#endif // LAUNCHERWINDOW_H
