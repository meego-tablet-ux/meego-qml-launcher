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
#include <QTranslator>
#include <X11/X.h>

class QDeclarativeView;

class LauncherWindow : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int winId READ winId NOTIFY winIdChanged)
    Q_PROPERTY(int actualOrientation  READ actualOrientation WRITE setActualOrientation)
    Q_PROPERTY(bool inhibitScreenSaver READ inhibitScreenSaver WRITE setInhibitScreenSaver)
    Q_PROPERTY(QStringList call READ getCall NOTIFY callChanged)

public:
    LauncherWindow(bool fullscreen, int width, int height, bool opengl, bool setSource = true, QWidget *parent = NULL);
    ~LauncherWindow();

    void forwardCall(const QStringList& parameters);
    QDeclarativeView* getDeclarativeView() {return view;}

    int actualOrientation() {
        return m_actualOrientation;
    }
    void setActualOrientation(int orientation);

    bool inhibitScreenSaver() {
        return m_inhibitScreenSaver;
    }
    void setInhibitScreenSaver(bool inhibit);

    QStringList getCall() {
        return m_call;
    }

signals:
    void call(const QStringList& parameters);
    void winIdChanged();
    void vkbHeight(int height);
    void callChanged();

public slots:
    void triggerSystemUIMenu();
    void goHome();
    int winId() const {
        return internalWinId();
    }
    void handleInputMethodAreaChanged(const QRect &newArea) {
        emit vkbHeight(newArea.height());
    }
    void dismissKeyboard();

private slots:
    void loadTranslators();

protected:
    bool event(QEvent * event);

private:
    QDeclarativeView *view;
    QString sharePath;
    QTranslator qtTranslator;
    QTranslator commonTranslator;
    QTranslator mediaTranslator;
    QTranslator appTranslator;
    int m_actualOrientation;
    bool m_inhibitScreenSaver;
    QStringList m_call;
};

#endif // LAUNCHERWINDOW_H
