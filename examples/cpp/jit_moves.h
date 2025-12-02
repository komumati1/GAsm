//
// Created by mateu on 22.11.2025.
//

#ifndef GASM_JIT_MOVES_H
#define GASM_JIT_MOVES_H

#include <cstdint>
#include <vector>

using run_fn2_t = void (*)(double* inputs, size_t inputLength,
                          double* registers, size_t registerLength,
                          double* constants, size_t constantLength,
                          double (*rng)());

run_fn2_t compile_moves(const std::vector<uint8_t>& program);

#endif //GASM_JIT_MOVES_H
