#include "GasmPython.h"
#include <vector>

// --------- Helper: convert Python list -> vector<double> ----------
static std::vector<double> toDoubleVector(PyObject* list) {
    std::vector<double> out;
    Py_ssize_t n = PyList_Size(list);
    out.reserve(n);
    for (Py_ssize_t i = 0; i < n; i++)
        out.push_back(PyFloat_AsDouble(PyList_GetItem(list, i)));
    return out;
}

static std::vector<uint8_t> toByteVector(PyObject* list) {
    std::vector<uint8_t> out;
    Py_ssize_t n = PyList_Size(list);
    out.reserve(n);
    for (Py_ssize_t i = 0; i < n; i++)
        out.push_back((uint8_t)PyLong_AsUnsignedLong(PyList_GetItem(list, i)));
    return out;
}

// ============================================================================
//                           PyGAsm methods
// ============================================================================

static void PyGAsm_dealloc(PyGAsm* self) {
    delete self->cpp;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* PyGAsm_new(PyTypeObject* type, PyObject*, PyObject*) {
    PyGAsm* self = (PyGAsm*)type->tp_alloc(type, 0);
    if (self) self->cpp = new GAsm();
    return (PyObject*)self;
}

// GAsm.setProgram(program)
static PyObject* PyGAsm_setProgram(PyGAsm* self, PyObject* args) {
    PyObject* list;
    if (!PyArg_ParseTuple(args, "O", &list))
        return nullptr;

    auto prog = toByteVector(list);
    self->cpp->setProgram(prog);

    Py_RETURN_NONE;
}

// GAsm.run(inputs)
static PyObject* PyGAsm_run(PyGAsm* self, PyObject* args) {
    PyObject* list;
    if (!PyArg_ParseTuple(args, "O", &list))
        return nullptr;

    auto vec = toDoubleVector(list);
    size_t r = self->cpp->run(vec);
    return PyLong_FromSize_t(r);
}

// GAsm.runAll(program, outputs)
static PyObject* PyGAsm_runAll(PyGAsm* self, PyObject* args) {
    PyObject* programList;
    PyObject* outputsList;

    if (!PyArg_ParseTuple(args, "OO", &programList, &outputsList))
        return nullptr;

    auto prog = toByteVector(programList);

    // convert outputs: list[list[float]]
    std::vector<std::vector<double>> outputs;
    Py_ssize_t outer = PyList_Size(outputsList);
    outputs.reserve(outer);

    for (Py_ssize_t i = 0; i < outer; i++)
        outputs.push_back(toDoubleVector(PyList_GetItem(outputsList, i)));

    double result = self->cpp->runAll(prog, outputs);
    return PyFloat_FromDouble(result);
}

// GAsm.setCNG(ptr)
static PyObject* PyGAsm_setCNG(PyGAsm* self, PyObject* args) {
    unsigned long long addr;
    if (!PyArg_ParseTuple(args, "K", &addr))
        return nullptr;

    self->cpp->setCNG((gen_fun_t)addr);
    Py_RETURN_NONE;
}

// GAsm.setRNG(ptr)
static PyObject* PyGAsm_setRNG(PyGAsm* self, PyObject* args) {
    unsigned long long addr;
    if (!PyArg_ParseTuple(args, "K", &addr))
        return nullptr;

    self->cpp->setRNG((gen_fun_t)addr);
    Py_RETURN_NONE;
}

// GAsm.evolve(inputs, targets)
static PyObject* PyGAsm_evolve(PyGAsm* self, PyObject* args) {
    PyObject* inList;
    PyObject* tarList;

    if (!PyArg_ParseTuple(args, "OO", &inList, &tarList))
        return nullptr;

    // inputs: list[list[float]]
    std::vector<std::vector<double>> inputs;
    std::vector<std::vector<double>> targets;

    Py_ssize_t nIn = PyList_Size(inList);
    inputs.reserve(nIn);
    for (Py_ssize_t i = 0; i < nIn; i++)
        inputs.push_back(toDoubleVector(PyList_GetItem(inList, i)));

    Py_ssize_t nTar = PyList_Size(tarList);
    targets.reserve(nTar);
    for (Py_ssize_t i = 0; i < nTar; i++)
        targets.push_back(toDoubleVector(PyList_GetItem(tarList, i)));

    self->cpp->evolve(inputs, targets);
    Py_RETURN_NONE;
}

// ============================================================================
//                             Method table
// ============================================================================

static PyMethodDef PyGAsm_methods[] = {
        {"setProgram", (PyCFunction)PyGAsm_setProgram, METH_VARARGS, "Set program"},
        {"run",        (PyCFunction)PyGAsm_run,        METH_VARARGS, "Run once"},
        {"runAll",     (PyCFunction)PyGAsm_runAll,     METH_VARARGS, "Run all"},
        {"setCNG",     (PyCFunction)PyGAsm_setCNG,     METH_VARARGS, "Set constant generator"},
        {"setRNG",     (PyCFunction)PyGAsm_setRNG,     METH_VARARGS, "Set RNG"},
        {"evolve",     (PyCFunction)PyGAsm_evolve,     METH_VARARGS, "Run evolution"},
        {nullptr, nullptr, 0, nullptr}
};

// ============================================================================
//                         Python type object
// ============================================================================

static PyTypeObject PyGAsmType = {
        PyVarObject_HEAD_INIT(NULL, 0)
};

static void init_PyGAsmType() {
    PyGAsmType.tp_name = "gasm.GAsm";
    PyGAsmType.tp_basicsize = sizeof(PyGAsm);
    PyGAsmType.tp_dealloc = (destructor)PyGAsm_dealloc;
    PyGAsmType.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGAsmType.tp_methods = PyGAsm_methods;
    PyGAsmType.tp_new = PyGAsm_new;
}


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

    if (PyType_Ready(&PyGAsmType) < 0)
        return nullptr;

    PyObject* m = PyModule_Create(&gasmModule);
    if (!m)
        return nullptr;

    Py_INCREF(&PyGAsmType);
    PyModule_AddObject(m, "GAsm", (PyObject*)&PyGAsmType);

    return m;
}

