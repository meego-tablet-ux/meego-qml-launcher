/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QtCore>
#include <QX11Info>
#include <cstdlib>

#include <stdio.h>

#include "launcheratoms.h"

static Atom appAtoms[ATOM_COUNT];
// Keep this in sync with the AtomType enum in atoms.h
static const char *appAtomNames[ATOM_COUNT] = {
    "_NET_ACTIVE_WINDOW",
    "UTF8_STRING"
};

void initAtoms ()
{
    Status status;
    Display *dpy = QX11Info::display ();

    status = XInternAtoms (dpy, (char **) appAtomNames, ATOM_COUNT,
                           False, appAtoms);
    if (status == 0) {
        fprintf (stderr, "Critical: Error getting atoms");
        abort ();
    }
}

Atom getAtom (AtomType type)
{
    return appAtoms[type];
}
