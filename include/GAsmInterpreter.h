//
// Created by mateu on 20.11.2025.
//

#ifndef GASM_GASMINTERPRETER_H
#define GASM_GASMINTERPRETER_H

#include <functional>
#include "xbyak.h"
#include "functions.h"

// function type returned by compile(...)
using run_fn_t = size_t (*)(double* inputs, size_t inputLength,
                            double* registers, size_t registerLength,
                            double (*constants)(),
                            double (*rng)(),
                            size_t maxProcessTime);

class GAsmInterpreter {
private:
    const std::vector<uint8_t>* _program;
    std::vector<double> _registers;
    Xbyak::CodeGenerator _code;
    run_fn_t _compiled;
public:
    [[nodiscard]] run_fn_t compile();
    explicit GAsmInterpreter(const std::vector<uint8_t>& program, size_t registerLength);
    explicit GAsmInterpreter(size_t registerLength);
    void setProgram(const std::vector<uint8_t>& program);
    void setRegisterLength(size_t registerLength);
    size_t run(std::vector<double> &inputs, size_t maxProcessTime);
    size_t runInterpreter(std::vector<double> &inputs, size_t maxProcessTime);
    size_t runCompiled(std::vector<double> &inputs, size_t maxProcessTime);

    bool useCompile = true;
    double (*cng)() = [](){ // TODO protect nullptr
        static size_t counter = 0;
        return (double) counter++;
    };
    double (*rng)() = [](){ // TODO protect nullptr
        static thread_local std::mt19937 engine(std::random_device{}());
        static std::uniform_real_distribution<double> dist(0, 1);
        return dist(engine);
    };
};


#endif //GASM_GASMINTERPRETER_H
