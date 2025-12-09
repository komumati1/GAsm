//
// Created by mateu on 08.12.2025.
//

#include "GasmModule.h"
#include <Python.h>
#include "GasmPython.h"
#include "HistPython.h"
#include "IndividualPython.h"
#include "EntryPython.h"

// ============================================================================
//                           Module definition
// ============================================================================

static PyModuleDef gasmModule = {
        PyModuleDef_HEAD_INIT,
        "gasm",
        "Python bindings for GAsm engine",
        -1,
        nullptr
};

PyMODINIT_FUNC PyInit_gasm(void) {
    init_PyGAsmType();
    init_PyIndividualType();
    init_PyHistType();
    init_PyEntryType();

    if (PyType_Ready(&PyGAsmType) < 0) return nullptr;
    if (PyType_Ready(&PyIndividualType) < 0) return nullptr;
    if (PyType_Ready(&PyHistType) < 0) return nullptr;
    if (PyType_Ready(&PyEntryType) < 0) return nullptr;

    PyObject* m = PyModule_Create(&gasmModule);
    if (!m)
        return nullptr;

    Py_INCREF(&PyGAsmType);
    Py_INCREF(&PyIndividualType);
    Py_INCREF(&PyHistType);
    Py_INCREF(&PyEntryType);

    PyModule_AddObject(m, "GAsm", (PyObject*)&PyGAsmType);
    PyModule_AddObject(m, "Individual", (PyObject*)&PyIndividualType);
    PyModule_AddObject(m, "Hist", (PyObject*)&PyHistType);
    PyModule_AddObject(m, "Entry", (PyObject*)&PyEntryType);

    return m;
}