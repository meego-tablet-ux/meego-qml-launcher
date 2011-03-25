/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#ifndef _LAUNCHER_ATOMS_H_
#define _LAUNCHER_ATOMS_H_

typedef enum _AtomType {
    ATOM_NET_ACTIVE_WINDOW,
    ATOM_UTF8_STRING,
    ATOM_COUNT
} AtomType;

void initAtoms ();
Atom getAtom (AtomType type);

#endif
