//
// Created by mateu on 08.12.2025.
//

#include "EntryPython.h"
#include "IndividualPython.h"

static PyObject* PyEntry_get_generation(PyEntry* self, void*) {
    return PyLong_FromLong(self->cpp->getGeneration());
}

static PyObject* PyEntry_get_bestFitness(PyEntry* self, void*) {
    return PyFloat_FromDouble(self->cpp->getBestFitness());
}

static PyObject* PyEntry_get_avgFitness(PyEntry* self, void*) {
    return PyFloat_FromDouble(self->cpp->getAvgFitness());
}

static PyObject* PyEntry_get_avgSize(PyEntry* self, void*) {
    return PyFloat_FromDouble(self->cpp->getAvgSize());
}

static PyObject* PyEntry_get_bestIndividual(PyEntry* self, void*) {
    Individual ind = self->cpp->getBestIndividual();
    return PyIndividual_newFromCPP(ind);
}

PyObject* PyEntry_newFromCPP(const Entry* entry) {
    PyEntry* obj = PyObject_New(PyEntry, &PyEntryType);
    if (!obj) return nullptr;
    obj->cpp = entry;
    return (PyObject*)obj;
}

static PyGetSetDef PyEntry_getset[] = {
        {"generation",   (getter)PyEntry_get_generation,    nullptr, "generation",    nullptr},
        {"best_fitness", (getter)PyEntry_get_bestFitness,   nullptr, "best fitness",  nullptr},
        {"avg_fitness",  (getter)PyEntry_get_avgFitness,    nullptr, "avg fitness",   nullptr},
        {"avg_size",     (getter)PyEntry_get_avgSize,       nullptr, "avg size",      nullptr},
        {"best",         (getter)PyEntry_get_bestIndividual,nullptr,"best individual",nullptr},
        {nullptr}
};

PyTypeObject PyEntryType = {
        PyVarObject_HEAD_INIT(NULL, 0)
};

void init_PyEntryType() {
    PyEntryType.tp_name = "gasm.Entry";
    PyEntryType.tp_basicsize = sizeof(PyEntry);
    PyEntryType.tp_flags = Py_TPFLAGS_DEFAULT;
    PyEntryType.tp_doc = "History object (read-only)";
    PyEntryType.tp_getset  = PyEntry_getset;
}

