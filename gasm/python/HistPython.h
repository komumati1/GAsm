//
// Created by mateu on 08.12.2025.
//

#ifndef GASM_HISTPYTHON_H
#define GASM_HISTPYTHON_H

#include <Python.h>
#include "Hist.h"

// Python object that wraps Hist
typedef struct {
    PyObject_HEAD
    Hist* cpp;
} PyHist;

extern PyTypeObject PyHistType;

PyObject* PyHist_newFromCPP(Hist*);

void init_PyHistType();


#endif //GASM_HISTPYTHON_H
