//
// Created by mateu on 08.12.2025.
//

#ifndef GASM_ENTRYPYTHON_H
#define GASM_ENTRYPYTHON_H

#include <Python.h>
#include "Entry.h"

typedef struct {
    PyObject_HEAD
    const Entry* cpp;   // pointer to existing Entry in Hist
} PyEntry;

extern PyTypeObject PyEntryType;

PyObject* PyEntry_newFromCPP(const Entry*);

void init_PyEntryType();

#endif //GASM_ENTRYPYTHON_H
