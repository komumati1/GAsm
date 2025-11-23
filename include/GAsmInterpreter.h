//
// Created by mateu on 20.11.2025.
//

#ifndef GASM_GASMINTERPRETER_H
#define GASM_GASMINTERPRETER_H

// function type returned by compile(...)
using run_fn_t = size_t (*)(double* inputs, size_t inputLength,
                          double* registers, size_t registerLength,
                          double (*constants)(),
                          double (*rng)());

// TODO
// register must not be length 0
// input must not be length 0

class GAsmInterpreter {
private:
    std::vector<uint8_t>& _program;
    std::vector<double> _registers;
    std::vector<double> _inputs;
    std::vector<double>& _constants;
    size_t _counter;
    size_t _constantsLength;
    size_t _maxProcessTime = 1000;
public:
    run_fn_t compile(const std::vector<uint8_t>& program);
    GAsmInterpreter(std::vector<uint8_t>& program, size_t registerLength, std::vector<double>& constants);  // TODO change constants to factory (function)
    void setProgram(std::vector<uint8_t>& program);
    size_t run(std::vector<double> &inputs, size_t maxProcessTime);
};


#endif //GASM_GASMINTERPRETER_H
