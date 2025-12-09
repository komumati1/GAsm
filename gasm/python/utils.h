//
// Created by mateu on 08.12.2025.
//

#ifndef GASM_UTILS_H
#define GASM_UTILS_H

#include <string>
#include "GAsmInterpreter.h"

std::unique_ptr<gen_fn_t> makeCNG(const std::string& spec, double start);

std::unique_ptr<gen_fn_t> makeRNG(const std::string& spec, double start);


#endif //GASM_UTILS_H
