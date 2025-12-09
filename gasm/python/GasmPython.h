#ifndef GASM_GASMPYTHON_H
#define GASM_GASMPYTHON_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "GAsm.h"    // your full class

// Python object that wraps GAsm
typedef struct {
    PyObject_HEAD
    GAsm* cpp;
} PyGAsm;

extern PyTypeObject PyGAsmType;

void init_PyGAsmType();

#endif
