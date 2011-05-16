/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QObject>
#include <cstdlib>

#include "meegoqmllauncher.h"

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        qWarning("USAGE: %s [options] APPNAME", argv[0]);
        std::exit(-1);
    }

    MeeGoQMLLauncher::prepareForLaunch();

    return MeeGoQMLLauncher::launch(argc, argv);
}
