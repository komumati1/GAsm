#include "GasmPython.h"
#include <Python.h>
#include <vector>
#include <iostream>
#include "HistPython.h"
#include "IndividualPython.h"
#include "utils.h"

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
    self->cpp->setFitnessFunction(std::make_unique<FitnessSumSub>());
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

    self->cpp->parallelEvolve(inputs, targets); /// TODO change between parallel and standard
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

static PyObject* PyGAsm_setSelection(PyGAsm* self, PyObject* args) {
    const char* mode;
    int param = 0;

    if (!PyArg_ParseTuple(args, "s|i", &mode, &param))
        return nullptr;

    std::string m(mode);

    if (m == "Tournament") self->cpp->setSelectionFunction(std::make_unique<TournamentSelection>(param));
    else if (m == "Truncation") self->cpp->setSelectionFunction(std::make_unique<TruncationSelection>(param));
    else if (m == "Boltzman") self->cpp->setSelectionFunction(std::make_unique<BoltzmannSelection>(param));
    else if (m == "Rank") self->cpp->setSelectionFunction(std::make_unique<RankSelection>());
    else if (m == "Roulette") self->cpp->setSelectionFunction(std::make_unique<RouletteSelection>());
    else {
        PyErr_SetString(PyExc_ValueError, "Invalid selection literal");
        return nullptr;
    }

    Py_RETURN_NONE;
}

static PyObject* PyGAsm_setGrow(PyGAsm* self, PyObject* args) {
    const char* mode;
    int param = 0;

    if (!PyArg_ParseTuple(args, "s|i", &mode, &param))
        return nullptr;

    std::string m(mode);

    if (m == "Size") self->cpp->setGrowFunction(std::make_unique<SizeGrow>(param));
    else if (m == "Full") self->cpp->setGrowFunction(std::make_unique<FullGrow>());
    else if (m == "Tree") self->cpp->setGrowFunction(std::make_unique<TreeGrow>(param));
    else {
        PyErr_SetString(PyExc_ValueError, "Invalid grow literal");
        return nullptr;
    }

    Py_RETURN_NONE;
}

static PyObject* PyGAsm_setMutation(PyGAsm* self, PyObject* arg) {
    if (!PyUnicode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "mutation must be a string literal");
        return nullptr;
    }

    std::string m = PyUnicode_AsUTF8(arg);

    if (m == "Hard") self->cpp->setMutationFunction(std::make_unique<HardMutation>());
    else if (m == "Soft") self->cpp->setMutationFunction(std::make_unique<SoftMutation>());
    else {
        PyErr_SetString(PyExc_ValueError, "Invalid mutation literal");
        return nullptr;
    }

    Py_RETURN_NONE;
}

static PyObject* PyGAsm_setCrossover(PyGAsm* self, PyObject* arg) {
    if (!PyUnicode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "crossover must be a string literal");
        return nullptr;
    }

    std::string m = PyUnicode_AsUTF8(arg);

    if (m == "OnePoint") self->cpp->setCrossoverFunction(std::make_unique<OnePointCrossover>());
    else if (m == "TwoPoint") self->cpp->setCrossoverFunction(std::make_unique<TwoPointCrossover>());
    else if (m == "TwoPointSize") self->cpp->setCrossoverFunction(std::make_unique<TwoPointSizeCrossover>());
    else if (m == "Uniform") self->cpp->setCrossoverFunction(std::make_unique<UniformPointCrossover>());
    else {
        PyErr_SetString(PyExc_ValueError, "Invalid crossover literal");
        return nullptr;
    }

    Py_RETURN_NONE;
}

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

// ============================================================================
//                             Method table
// ============================================================================

static PyMethodDef PyGAsm_methods[] = {
        {"setProgram", (PyCFunction)PyGAsm_setProgram, METH_VARARGS, "Set program"},
        {"run",        (PyCFunction)PyGAsm_run,        METH_VARARGS, "Run once"},
        {"runAll",     (PyCFunction)PyGAsm_runAll,     METH_VARARGS, "Run all"},
        {"evolve",     (PyCFunction)PyGAsm_evolve,     METH_VARARGS, "Run evolution"},
        {"save2File", (PyCFunction)PyGAsm_save2File, METH_VARARGS, "Save engine state to JSON file"},
        {"fromJson",  (PyCFunction)PyGAsm_fromJson,  METH_VARARGS | METH_STATIC, "Load engine state from JSON file"},
        {"setSelection", (PyCFunction)PyGAsm_setSelection, METH_VARARGS, "Set selection mode"},
        {"setGrow",      (PyCFunction)PyGAsm_setGrow,      METH_VARARGS, "Set grow mode"},
        {"setMutation",  (PyCFunction)PyGAsm_setMutation,  METH_O,       "Set mutation type"},
        {"setCrossover", (PyCFunction)PyGAsm_setCrossover, METH_O,       "Set crossover type"},
        {"set_cng",    (PyCFunction)PyIndividual_setCNG,     METH_VARARGS, "Set CNG generator"},
        {"set_rng",    (PyCFunction)PyIndividual_setRNG,     METH_VARARGS, "Set RNG generator"},
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

static PyObject* PyGAsm_get_best(PyGAsm* self, void*) {
    Individual best = self->cpp->getBestIndividual();
    return PyIndividual_newFromCPP(best);  // returns PyIndividual*
}

static PyObject* PyGAsm_get_hist(PyGAsm* self, void*) {
    return PyHist_newFromCPP(&self->cpp->hist);  // depends on your field name
}

static PyObject* PyGAsm_get_outputFolder(PyGAsm* self, void*) {
    return PyUnicode_FromString(self->cpp->outputFolder.c_str());
}

static int PyGAsm_set_outputFolder(PyGAsm* self, PyObject* val, void*) {
    if (!PyUnicode_Check(val)) {
        PyErr_SetString(PyExc_TypeError, "outputFolder must be a string");
        return -1;
    }
    self->cpp->outputFolder = PyUnicode_AsUTF8(val);
    return 0;
}

static PyObject* PyGAsm_get_nanPenalty(PyGAsm* self, void*) {
    return PyFloat_FromDouble(self->cpp->nanPenalty);
}

static int PyGAsm_set_nanPenalty(PyGAsm* self, PyObject* val, void*) {
    if (!PyFloat_Check(val) && !PyLong_Check(val)) {
        PyErr_SetString(PyExc_TypeError, "nanPenalty must be a number");
        return -1;
    }
    self->cpp->nanPenalty = PyFloat_AsDouble(val);
    return 0;
}

static PyObject* PyGAsm_get_useCompile(PyGAsm* self, void*) {
    if (self->cpp->getCompile())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static int PyGAsm_set_useCompile(PyGAsm* self, PyObject* val, void*) {
    int isTrue = PyObject_IsTrue(val);
    if (isTrue < 0) return -1;
    self->cpp->setCompile((bool)isTrue);
    return 0;
}

static PyObject* PyGAsm_get_checkpointInterval(PyGAsm* self, void*) {
    return PyLong_FromSize_t(self->cpp->checkPointInterval);
}

static int PyGAsm_set_checkpointInterval(PyGAsm* self, PyObject* val, void*) {
    self->cpp->checkPointInterval = PyLong_AsSize_t(val);
    return (PyErr_Occurred() ? -1 : 0);
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
        {"best",            (getter)PyGAsm_get_best,            nullptr,                            "best individual", nullptr},
        {"hist",            (getter)PyGAsm_get_hist,            nullptr,                            "history", nullptr},
        {"outputFolder",    (getter)PyGAsm_get_outputFolder,    (setter)PyGAsm_set_outputFolder,    "output folder", nullptr},
        {"nanPenalty",      (getter)PyGAsm_get_nanPenalty,      (setter)PyGAsm_set_nanPenalty,      "NaN penalty", nullptr},
        {"useCompile",      (getter)PyGAsm_get_useCompile,      (setter)PyGAsm_set_useCompile,      "JIT compile flag", nullptr},
        {"checkpointInterval", (getter)PyGAsm_get_checkpointInterval, (setter)PyGAsm_set_checkpointInterval, "checkpoint interval", nullptr},
        {nullptr}
};


// ============================================================================
//                         Python type object
// ============================================================================

PyTypeObject PyGAsmType = {
        PyVarObject_HEAD_INIT(NULL, 0)
};

void init_PyGAsmType() {
    PyGAsmType.tp_name = "gasm.GAsm";
    PyGAsmType.tp_basicsize = sizeof(PyGAsm);
    PyGAsmType.tp_dealloc = (destructor)PyGAsm_dealloc;
    PyGAsmType.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGAsmType.tp_methods = PyGAsm_methods;
    PyGAsmType.tp_new = PyGAsm_new;
    PyGAsmType.tp_getset = PyGAsm_getset;
}

