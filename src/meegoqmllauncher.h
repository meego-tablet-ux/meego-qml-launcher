/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef MEEGO_QML_LAUNCHER_H
#define MEEGO_QML_LAUNCHER_H

class MeeGoQMLLauncher
{
public:

    static void prepareForLaunch();

    static int launch(int argc, char **argv);

    static const char * preinitialisedAppNamePrefix;

private:
    MeeGoQMLLauncher();
};

#endif
