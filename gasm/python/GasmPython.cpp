#include "GasmPython.h"
#include <Python.h>
#include <vector>
#include <iostream>

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

static PyTypeObject PyGAsmType = {
        PyVarObject_HEAD_INIT(NULL, 0)
};

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

//    for (int i = 0; i < inputs.size(); i++)
//    {
//        for (int j = 0; j < inputs[i].size(); j++)
//        {
//            std::cout << inputs[i][j] << std::endl;
//        }
//    }
//
//    for (int i = 0; i < targets.size(); i++)
//    {
//        for (int j = 0; j < targets[i].size(); j++)
//        {
//            std::cout << targets[i][j] << std::endl;
//        }
//    }

    self->cpp->evolve(inputs, targets);
    Py_RETURN_NONE;
}

// GAsm.save2File(filename)
static PyObject* PyGAsm_save2File(PyGAsm* self, PyObject* args) {
    const char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return nullptr;

    self->cpp->save2File(filename);
    Py_RETURN_NONE;
}

// GAsm.fromJson(filename)  --> returns new GAsm object
static PyObject* PyGAsm_fromJson(PyObject*, PyObject* args) {
    const char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return nullptr;

    GAsm* cppObj = new GAsm(filename);
    if (!cppObj)
        Py_RETURN_NONE;

    PyGAsm* pyObj = PyObject_New(PyGAsm, &PyGAsmType);
    pyObj->cpp = cppObj;
    return (PyObject*)pyObj;
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
        {"save2File", (PyCFunction)PyGAsm_save2File, METH_VARARGS, "Save engine state to JSON file"},
        {"fromJson",  (PyCFunction)PyGAsm_fromJson,  METH_VARARGS | METH_STATIC, "Load engine state from JSON file"},
        {nullptr, nullptr, 0, nullptr}
};

// ============================================================================
//                           PyGAsm attributes
// ============================================================================

static PyObject* PyGAsm_get_populationSize(PyGAsm* self, void*) {
    return PyLong_FromUnsignedLong(self->cpp->populationSize);
}

static int PyGAsm_set_populationSize(PyGAsm* self, PyObject* val, void*) {
    self->cpp->populationSize = PyLong_AsUnsignedLong(val);
    return 0;
}

static PyObject* PyGAsm_get_individualMaxSize(PyGAsm* self, void*) {
    return PyLong_FromUnsignedLong(self->cpp->individualMaxSize);
}

static int PyGAsm_set_individualMaxSize(PyGAsm* self, PyObject* val, void*) {
    self->cpp->individualMaxSize = PyLong_AsUnsignedLong(val);
    return 0;
}

static PyObject* PyGAsm_get_mutationProbability(PyGAsm* self, void*) {
    return PyFloat_FromDouble(self->cpp->mutationProbability);
}

static int PyGAsm_set_mutationProbability(PyGAsm* self, PyObject* val, void*) {
    self->cpp->mutationProbability = PyFloat_AsDouble(val);
    return 0;
}

static PyObject* PyGAsm_get_crossoverProbability(PyGAsm* self, void*) {
    return PyFloat_FromDouble(self->cpp->crossoverProbability);
}

static int PyGAsm_set_crossoverProbability(PyGAsm* self, PyObject* val, void*) {
    self->cpp->crossoverProbability = PyFloat_AsDouble(val);
    return 0;
}

static PyObject* PyGAsm_get_maxGenerations(PyGAsm* self, void*) {
    return PyLong_FromUnsignedLong(self->cpp->maxGenerations);
}

static int PyGAsm_set_maxGenerations(PyGAsm* self, PyObject* val, void*) {
    self->cpp->maxGenerations = PyLong_AsUnsignedLong(val);
    return 0;
}

static PyObject* PyGAsm_get_goalFitness(PyGAsm* self, void*) {
    return PyFloat_FromDouble(self->cpp->goalFitness);
}

static int PyGAsm_set_goalFitness(PyGAsm* self, PyObject* val, void*) {
    self->cpp->goalFitness = PyFloat_AsDouble(val);
    return 0;
}

static PyObject* PyGAsm_get_minimize(PyGAsm* self, void*) {
    return PyBool_FromLong(self->cpp->minimize ? 1 : 0);
}

static int PyGAsm_set_minimize(PyGAsm* self, PyObject* val, void*) {
    self->cpp->minimize = (bool)val;
    return 0;
}

static PyObject* PyGAsm_get_registerLength(PyGAsm* self, void*) {
    return PyLong_FromSize_t(self->cpp->getRegisterLength());
}

static int PyGAsm_set_registerLength(PyGAsm* self, PyObject* val, void*) {
    self->cpp->setRegisterLength(PyLong_AsSize_t(val));
    return 0;
}

static PyObject* PyGAsm_get_maxProcessTime(PyGAsm* self, void*) {
    return PyLong_FromSize_t(self->cpp->maxProcessTime);
}

static int PyGAsm_set_maxProcessTime(PyGAsm* self, PyObject* val, void*) {
    self->cpp->maxProcessTime = PyLong_AsSize_t(val);
    return 0;
}


// ============================================================================
//                           Attributes table
// ============================================================================
static PyGetSetDef PyGAsm_getset[] = {
        {"populationSize",
                (getter)PyGAsm_get_populationSize,
                (setter)PyGAsm_set_populationSize,
                "population size", nullptr},

        {"individualMaxSize",
                (getter)PyGAsm_get_individualMaxSize,
                (setter)PyGAsm_set_individualMaxSize,
                "max individual size", nullptr},

        {"mutationProbability",
                (getter)PyGAsm_get_mutationProbability,
                (setter)PyGAsm_set_mutationProbability,
                "mutation probability", nullptr},

        {"crossoverProbability",
                (getter)PyGAsm_get_crossoverProbability,
                (setter)PyGAsm_set_crossoverProbability,
                "crossover probability", nullptr},

        {"maxGenerations",
                (getter)PyGAsm_get_maxGenerations,
                (setter)PyGAsm_set_maxGenerations,
                "maximum generations", nullptr},

        {"goalFitness",
                (getter)PyGAsm_get_goalFitness,
                (setter)PyGAsm_set_goalFitness,
                "goal fitness", nullptr},

        {"minimize",
                (getter)PyGAsm_get_minimize,
                (setter)PyGAsm_set_minimize,
                "minimize mode", nullptr},

        {"registerLength",
                (getter)PyGAsm_get_registerLength,
                (setter)PyGAsm_set_registerLength,
                "number of registers", nullptr},
        {"maxProcessTime",
                (getter) PyGAsm_get_maxProcessTime,
                (setter) PyGAsm_set_maxProcessTime,
                "max process time", nullptr},

        {nullptr}
};


// ============================================================================
//                         Python type object
// ============================================================================

static void init_PyGAsmType() {
    PyGAsmType.tp_name = "gasm.GAsm";
    PyGAsmType.tp_basicsize = sizeof(PyGAsm);
    PyGAsmType.tp_dealloc = (destructor)PyGAsm_dealloc;
    PyGAsmType.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGAsmType.tp_methods = PyGAsm_methods;
    PyGAsmType.tp_new = PyGAsm_new;
    PyGAsmType.tp_getset = PyGAsm_getset;
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

