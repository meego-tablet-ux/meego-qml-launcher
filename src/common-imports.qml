import Qt 4.7
import QtQuick 1.0
import MeeGo.Components 0.1
import MeeGo.Labs.Components 0.1 as Labs

Item {
    width: screenWidth
    height: screenHeight

    property alias appItem: appLauncherAppLoader.item

    Loader {
        id: appLauncherAppLoader

        // The mainWindow's appSource will be an empty string
        // until the invoker gets a command to turn into a specific
        // application
        onSourceChanged: {
            if (source != "")
            {
                // We started in software rendering since it's actually
                // faster to do the initial paint in software mode, but
                // once we get here then we switch back over to gl
                mainWindow.switchToGLRendering();
            }
        }
        // Once we have finished loading the app source then
        // remove the splash content
        onLoaded: {
            if (status == Loader.Ready)
            {
                appLauncherSplashLoader.sourceComponent = undefined;
            }
        }
    }

    Loader {
        id: appLauncherSplashLoader
    }

    Component {
        id:  appLauncherSplashComponent
        Rectangle {
            width: screenWidth
            height: screenHeight
            color: "black"
            Connections {
                target: qApp
                onForegroundWindowChanged: {
                    if (qApp.foregroundWindow == mainWindow.winId)
                    {
                        appLauncherAppLoader.source = mainWindow.appSource;
                    }
                }
            }
            Image {
                anchors.centerIn: parent
                source: qApp.splashImage
            }
        }
    }
    Component.onCompleted: {
        appLauncherSplashLoader.sourceComponent = appLauncherSplashComponent;
    }
}

