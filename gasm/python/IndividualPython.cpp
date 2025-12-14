//
// Created by mateu on 08.12.2025.
//

#include "IndividualPython.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "utils.h"

// ---------------------- Deallocation -------------------------

static void PyIndividual_dealloc(PyIndividual* self) {
    delete self->cpp;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// ---------------------- __init__ -------------------------

static int PyIndividual_init(PyIndividual* self, PyObject* args, PyObject* kw) {
    const char* code_str = nullptr;
    PyObject* bytecode_list = nullptr;

    static const char* kwlist[] = { "code_str", "bytecode", nullptr };

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|sO", (char**)kwlist,
                                     &code_str, &bytecode_list)) {
        return -1;
    }

    self->cpp = nullptr;

    if (code_str) {
        self->cpp = new Individual(std::string(code_str));
    } else if (bytecode_list) {
        if (!PyList_Check(bytecode_list)) {
            PyErr_SetString(PyExc_TypeError, "bytecode must be a list of ints");
            delete self->cpp;
            self->cpp = nullptr;
            return -1;
        }
        std::vector<uint8_t> bc;
        Py_ssize_t n = PyList_Size(bytecode_list);
        bc.reserve(n);
        for (Py_ssize_t i = 0; i < n; i++) {
            PyObject* item = PyList_GetItem(bytecode_list, i);
            if (!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "bytecode list must contain ints");
                delete self->cpp;
                self->cpp = nullptr;
                return -1;
            }
            long v = PyLong_AsLong(item);
            if (v < 0 || v > 255) {
                PyErr_SetString(PyExc_ValueError, "bytecode values must be in [0,255]");
                delete self->cpp;
                self->cpp = nullptr;
                return -1;
            }
            bc.push_back((uint8_t)v);
        }
        self->cpp = new Individual(bc);
    }

    return 0;
}

PyObject* PyIndividual_newFromCPP(const Individual& ind) {
    PyIndividual* obj = PyObject_New(PyIndividual, &PyIndividualType);
    if (!obj) return nullptr;
    obj->cpp = new Individual(ind);   // deep copy so Python owns it
    return (PyObject*)obj;
}

// ---------------------- Methods -------------------------

static PyObject* PyIndividual_run(PyIndividual* self, PyObject* arg) {
    if (!PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "run() requires a list of floats");
        return nullptr;
    }

    std::vector<double> inputs;
    Py_ssize_t n = PyList_Size(arg);

    inputs.reserve(n);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject* item = PyList_GetItem(arg, i);
        if (!PyFloat_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "run() list must contain floats");
            return nullptr;
        }
        inputs.push_back(PyFloat_AsDouble(item));
    }
    // FIXME this is such a dirty fix
    std::vector<uint8_t> code = self->cpp->getBytecode();
    GAsmInterpreter jit = GAsmInterpreter(code, self->cpp->getRegisterLength());
    jit.useCompile = self->cpp->getCompile();
    jit.setCng(std::make_unique<gen_fn_t>(self->cpp->getCNG()));
    jit.setRng(std::make_unique<gen_fn_t>(self->cpp->getRNG()));
    size_t result = jit.run(inputs, self->cpp->maxProcessTime);
//    size_t result = self->cpp->run(inputs);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyList_SetItem(arg, i, PyFloat_FromDouble(inputs[i]));
    }

    return PyLong_FromSize_t(result);
}

static PyObject* PyIndividual_toString(PyIndividual* self,
                                       PyObject* /*unused*/) {
    return PyUnicode_FromString(self->cpp->toString().c_str());
}

// ---------------------- Getters / Setters -------------------------

static PyObject* PyIndividual_get_maxProcessTime(PyIndividual* self, void*) {
    return PyLong_FromSize_t(self->cpp->maxProcessTime);
}

static int PyIndividual_set_maxProcessTime(PyIndividual* self, PyObject* value, void*) {
    if (!PyLong_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "maxProcessTime must be int");
        return -1;
    }
    self->cpp->maxProcessTime = PyLong_AsSize_t(value);
    return 0;
}

// ----- registerLength -----

static PyObject* PyIndividual_get_registerLength(PyIndividual* self, void*) {
    return PyLong_FromSize_t(self->cpp->getRegisterLength());
}

static int PyIndividual_set_registerLength(PyIndividual* self, PyObject* value, void*) {
    if (!PyLong_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "registerLength must be int");
        return -1;
    }
    self->cpp->setRegisterLength(PyLong_AsSize_t(value));
    return 0;
}

// ----- compile -----

static PyObject* PyIndividual_get_compile(PyIndividual* self, void*) {
    if (self->cpp->getCompile())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static int PyIndividual_set_compile(PyIndividual* self, PyObject* value, void*) {
    int isTrue = PyObject_IsTrue(value);
    if (isTrue < 0) return -1;
    self->cpp->setCompile((bool)isTrue);
    return 0;
}

// ----- CNG -----

static PyObject* PyIndividual_setCNG(PyIndividual* self, PyObject* args) {
    const char* spec = nullptr;
    double start = 0.0;

    if (!PyArg_ParseTuple(args, "sd", &spec, &start)) {
        PyErr_SetString(PyExc_TypeError, "set_cng(spec: str, start: float)");
        return nullptr;
    }

    try {
        self->cpp->setCNG(makeCNG(std::string(spec), start));
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }

    Py_RETURN_NONE;
}

static PyObject* PyIndividual_setRNG(PyIndividual* self, PyObject* args) {
    const char* spec = nullptr;
    double start = 0.0;

    if (!PyArg_ParseTuple(args, "sd", &spec, &start)) {
        PyErr_SetString(PyExc_TypeError, "set_rng(spec: str, start: float)");
        return nullptr;
    }

    try {
        self->cpp->setRNG(makeRNG(std::string(spec), start));
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }

    Py_RETURN_NONE;
}


// ---------------------- Method table -------------------------

static PyMethodDef PyIndividual_methods[] = {
        {"run",        (PyCFunction)PyIndividual_run,        METH_O,       "Run the individual"},
        {"toString",   (PyCFunction)PyIndividual_toString,   METH_NOARGS,  "Return bytecode textual form"},
        {"set_cng",    (PyCFunction)PyIndividual_setCNG,     METH_VARARGS, "Set CNG generator"},
        {"set_rng",    (PyCFunction)PyIndividual_setRNG,     METH_VARARGS, "Set RNG generator"},
        {nullptr}
};


// ---------------------- Get/Set table -------------------------

static PyGetSetDef PyIndividual_getset[] = {
        {"maxProcessTime",  (getter)PyIndividual_get_maxProcessTime,
                (setter)PyIndividual_set_maxProcessTime, "max process time"},
        {"registerLength",  (getter)PyIndividual_get_registerLength,
                (setter)PyIndividual_set_registerLength, "register length"},
        {"compile",         (getter)PyIndividual_get_compile,
                (setter)PyIndividual_set_compile,        "JIT compile flag"},
        {nullptr}
};

// ---------------------- Module initialization -------------------------

PyTypeObject PyIndividualType = {
        PyVarObject_HEAD_INIT(NULL, 0)
};

void init_PyIndividualType() {
    PyIndividualType.tp_name      = "gasm.Individual";
    PyIndividualType.tp_basicsize = sizeof(PyIndividual);
    PyIndividualType.tp_dealloc   = (destructor)PyIndividual_dealloc;
    PyIndividualType.tp_flags     = Py_TPFLAGS_DEFAULT;
    PyIndividualType.tp_doc       = "C++ Individual wrapper";
    PyIndividualType.tp_methods   = PyIndividual_methods;
    PyIndividualType.tp_getset    = PyIndividual_getset;
    PyIndividualType.tp_init      = (initproc)PyIndividual_init;
    PyIndividualType.tp_new       = PyType_GenericNew;
}
