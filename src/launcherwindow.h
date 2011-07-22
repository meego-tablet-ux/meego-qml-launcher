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
#include <QFileSystemWatcher>
#include <QTranslator>

namespace meego
{
    class Locale;
}

class LauncherWindow : public QDeclarativeView
{
    Q_OBJECT
    Q_PROPERTY(int winId READ winId NOTIFY winIdChanged)
    Q_PROPERTY(int actualOrientation  READ actualOrientation WRITE setActualOrientation)
    Q_PROPERTY(bool inhibitScreenSaver READ inhibitScreenSaver WRITE setInhibitScreenSaver)
    Q_PROPERTY(QStringList call READ getCall NOTIFY callChanged)
    Q_PROPERTY(QString debugInfo READ getDebugInfo NOTIFY debugInfoChanged);
    Q_PROPERTY(QString appSource READ getAppSource NOTIFY appSourceChanged);

public:
    LauncherWindow(bool fullscreen, int width, int height, bool opengl, bool doSetSource = true, QWidget *parent = NULL);
    ~LauncherWindow();

    void forwardCall(const QStringList& parameters);

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

    QString getDebugInfo() const {
        if (m_debugInfoEnabled)
            return m_debugInfo;
        else
            return QString();
    }

    QString getAppSource() const {
        return sharePath;
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
    void debugInfoChanged();
    void retranslateUi();
    void appSourceChanged();

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
    void switchToGLRendering();
    void switchToSoftwareRendering();

private slots:
    void loadCommonTranslators();
    void loadAppTranslators();
    void localeChanged();
    void updateOrientationSensorOn();
    void doSwitchToGLRendering();
    void debugDirChanged(const QString);
    void debugFileChanged(const QString);
    void loadScene();

protected:
    bool event(QEvent * event);
    void keyPressEvent ( QKeyEvent * event );

    void init(bool fullscreen, int width, int height, bool opengl,
              bool setSource = true);

private:
    void setEnableDebugInfo(bool);

    QString sharePath;
    QTranslator qtTranslator;
    QTranslator componentsTranslator;
    QTranslator labsTranslator;
    QTranslator mediaTranslator;
    QTranslator appTranslator;
    int m_actualOrientation;
    bool m_inhibitScreenSaver;
    QStringList m_call;
    bool m_useOpenGl;
    bool m_usingGl;
    meego::Locale* locale;

    bool m_debugInfoEnabled;
    QString m_debugInfo;
    QFileSystemWatcher m_debugInfoFileWatcher;

    bool m_pendingLoadScene;

    friend class MeeGoQMLLauncher;
};

#endif // LAUNCHERWINDOW_H
