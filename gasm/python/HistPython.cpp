#include "HistPython.h"
#include "EntryPython.h"

// --------- Sequence protocol ---------

static Py_ssize_t PyHist_len(PyObject* self) {
    auto* h = (PyHist*)self;
    return h->cpp->getEntries().size();
}

static PyObject* PyHist_getitem(PyObject* self, Py_ssize_t idx) {
    auto* h = (PyHist*)self;
    const auto& vec = h->cpp->getEntries();
    if (idx < 0 || idx >= (Py_ssize_t)vec.size()) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return nullptr;
    }
    return PyEntry_newFromCPP(&vec[idx]);
}

static PySequenceMethods PyHist_seq = {
        .sq_length = PyHist_len,
        .sq_item   = PyHist_getitem
};

// --------- Constructor helper ---------

PyObject* PyHist_newFromCPP(Hist* hist) {
    PyHist* obj = PyObject_New(PyHist, &PyHistType);
    if (!obj) return nullptr;
    obj->cpp = hist;
    return (PyObject*)obj;
}

// --------- Type definition ---------

static PyMethodDef PyHist_methods[] = {
        {nullptr}
};

static PyGetSetDef PyHist_getset[] = {
        {nullptr}
};

// --------- Init function ---------

PyTypeObject PyHistType = {
        PyVarObject_HEAD_INIT(NULL, 0)
};

void init_PyHistType() {
    PyHistType.tp_name = "gasm.Hist";
    PyHistType.tp_basicsize = sizeof(PyHist);
    PyHistType.tp_flags = Py_TPFLAGS_DEFAULT;
    PyHistType.tp_doc = "History object (read-only)";
    PyHistType.tp_methods = PyHist_methods;
    PyHistType.tp_getset  = PyHist_getset;
    PyHistType.tp_as_sequence = &PyHist_seq;

    if (PyType_Ready(&PyHistType) < 0)
        throw std::runtime_error("PyHistType init failed");
}
