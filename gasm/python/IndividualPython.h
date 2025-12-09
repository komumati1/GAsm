//
// Created by mateu on 08.12.2025.
//

#ifndef GASM_INDIVIDUALPYTHON_H
#define GASM_INDIVIDUALPYTHON_H

#include <Python.h>
#include "Individual.h"

// Python object that wraps Hist
typedef struct {
    PyObject_HEAD
    Individual* cpp;
} PyIndividual;

extern PyTypeObject PyIndividualType ;

PyObject* PyIndividual_newFromCPP(const Individual& ind);

void init_PyIndividualType();


#endif //GASM_INDIVIDUALPYTHON_H
