//
// Created by mateu on 20.11.2025.
//

#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include "GAsmParser.h"
#include "GAsmInterpreter.h"

GAsmInterpreter::GAsmInterpreter(const std::vector<uint8_t>& program, size_t registerLength)
  : _program(&program),
    _registers(registerLength),
    _compiled(nullptr),
    _code(1, Xbyak::AutoGrow) {
    if (registerLength == 0) {
        throw std::invalid_argument("Register length should be greater than 0");
    }
}

GAsmInterpreter::GAsmInterpreter(size_t registerLength)
  : _program(nullptr),
    _registers(registerLength),
    _compiled(nullptr),
    _code(1, Xbyak::AutoGrow) {
    if (registerLength == 0) {
        throw std::invalid_argument("Register length should be greater than 0");
    }
}

void GAsmInterpreter::setProgram(const std::vector<uint8_t>& program) {
    _program = &program;
    _code.reset();
    _compiled = nullptr;
}

void GAsmInterpreter::setRegisterLength(size_t registerLength) {
    _registers.resize(registerLength);
}

size_t GAsmInterpreter::run(std::vector<double> &inputs, size_t maxProcessTime) {
    if (useCompile) {
        return runCompiled(inputs, maxProcessTime);
    } else {
        return runInterpreter(inputs, maxProcessTime);
    }
}

size_t GAsmInterpreter::runInterpreter(std::vector<double> &inputs, size_t maxProcessTime) {
    if (inputs.empty()) {
        throw std::invalid_argument("Input length should be greater than 0");
    }
    if (_program == nullptr) {
        throw std::invalid_argument("Program is not set.");
    }
    std::fill(_registers.begin(), _registers.end(), 0);
    size_t inputLength = inputs.size();
    size_t registerLength = _registers.size();
    size_t P = 0;          // Program pointer
    double A = 0;          // Accumulator
    std::vector<size_t> pointerStack(0);
    std::vector<uint8_t> instructionStack(0);
    std::vector<size_t> pStack(0);
    size_t processTime = 0;
    bool skipToEnd = false;

    for (size_t i = 0; i < _program->size(); i++) {
        const uint8_t& opcode = _program->operator[](i);
        switch (opcode) {

            // ===== MOV =====
            case MOV_P_A:  // P = A
                P = static_cast<int>(A);
                break;

            case MOV_A_P:  // A = P
                A = static_cast<double>(P);
                break;

            case MOV_A_R:  // A = R[P]
                A = _registers[P % registerLength];
                break;

            case MOV_A_I:  // A = I[P]
                A = inputs[P % inputLength];
                break;

            case MOV_R_A:  // R = A
                _registers[P % registerLength] = A;
                break;

            case MOV_I_A:  // I[P] = A
                inputs[P % inputLength] = A;
                break;

            // ===== ARITHMETIC (R) =====
            case ADD_R: A += _registers[P % registerLength]; break;
            case SUB_R: A -= _registers[P % registerLength]; break;
            case DIV_R: A /= _registers[P % registerLength]; break;
            case MUL_R: A *= _registers[P % registerLength]; break;
            case SIN_R: A = sin(_registers[P % registerLength]); break;
            case COS_R: A = cos(_registers[P % registerLength]); break;
            case EXP_R: A = exp(_registers[P % registerLength]); break;


            // ===== ARITHMETIC (I) =====
            case ADD_I: A += inputs[P % inputLength]; break;
            case SUB_I: A -= inputs[P % inputLength]; break;
            case DIV_I: A /= inputs[P % inputLength]; break;
            case MUL_I: A *= inputs[P % inputLength]; break;
            case SIN_I: A = sin(inputs[P % inputLength]); break;
            case COS_I: A = cos(inputs[P % inputLength]); break;
            case EXP_I: A = exp(inputs[P % inputLength]); break;


            // ===== UNARY =====
            case INC: P++; break;
            case DEC: P--; break;
            case RES: P = 0; break;
            case SET: A = cng(); break;


            // ===== LOOPS =====
            case FOR:
                P = 0;   // Start loop at index 0
                pointerStack.push_back(i);  // push_back instruction
                instructionStack.push_back(FOR); // push_back current loop version
                pStack.push_back(P);        // save pointer
                break;

            case LOP_A:
                if (A < inputs[P % inputLength]) {  // this has to be changed in 2 places
                    instructionStack.push_back(LOP_A);
                    pointerStack.push_back(i);
                } else {
                    // skip till the END symbol
                    skipToEnd = true;
                }
                break;

            case LOP_P:
                if (P < inputLength) {  // this has to be changed in 2 places
                    instructionStack.push_back(LOP_P);
                    pointerStack.push_back(i);
                } else {
                    // skip till the END symbol
                    skipToEnd = true;
                }
                break;


            // ===== CONDITIONAL JUMPS =====
            case JMP_I:
                if (A >= inputs[P % inputLength]) {
                    // skip till the END symbol
                    skipToEnd = true;
                }
                break;

            case JMP_R:
                if (A >= _registers[P % registerLength]) {
                    // skip till the END symbol
                    skipToEnd = true;
                }
                break;

            case JMP_P:
                if ((double)P >= A) {
                    // skip till the END symbol
                    skipToEnd = true;
                }
                break;


            // ===== TERMINATION / RANDOM =====
            case END:
                if (!instructionStack.empty()) { //
                    uint8_t instruction = instructionStack.back();
                    switch (instruction) {
                        case FOR:
                            P = ++pStack.back();
                            if (P < inputLength) {
                                i = pointerStack.back();
                            } else {
                                instructionStack.pop_back();
                                pStack.pop_back();
                                pointerStack.pop_back();
                            }
                            break;
                        case LOP_A:
                            if (A < inputs[P % inputLength]) {  // this has to be changed in 2 places
                                i = pointerStack.back();
                            } else {
                                instructionStack.pop_back();
                                pointerStack.pop_back();
                            }
                            break;
                        case LOP_P:
                            if (P < inputLength) {  // this has to be changed in 2 places
                                i = pointerStack.back();
                            } else {
                                instructionStack.pop_back();
                                pointerStack.pop_back();
                            }
                            break;
                        default:
                            break;
                    }
                }
                break;

            case RNG:
                A = rng();
                break;


            default:
                // Unknown opcode → do nothing
                break;
        }
        processTime++;
        if (processTime > maxProcessTime) {
            break;
        }
        if (skipToEnd) {
            for (int endCounter = 0; i < _program->size(); i++) {
                const uint8_t &instruction = _program->operator[](i);
                if (FOR <= instruction && instruction <= JMP_P) { // instruction with END
                    endCounter++;
                } else if (instruction == END) {
                    if (endCounter == 0) {
                        break;
                    } else {
                        endCounter--;
                    }
                }
            }
            skipToEnd = false;
        }
    }
    return processTime;
}

size_t GAsmInterpreter::runCompiled(std::vector<double> &inputs, size_t maxProcessTime) {
    if (inputs.empty()) {
        throw std::invalid_argument("Input length should be greater than 0");
    }
    if (_program == nullptr) {
        throw std::invalid_argument("Program is not set.");
    }
    if (_compiled == nullptr) {
        _compiled = compile();
    }
    std::fill(_registers.begin(), _registers.end(), 0);
    return _compiled(inputs.data(), inputs.size(), _registers.data(), _registers.size(), cng, rng, maxProcessTime);
}

