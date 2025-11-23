//
// Created by mateu on 20.11.2025.
//

#include <vector>
#include <cstdint>
#include <cmath>
#include <random>
#include "GAsmInterpreter.h"
#include "GAsmParser.h"

GAsmInterpreter::GAsmInterpreter(std::vector<uint8_t> &program, size_t registerLength,
                                 std::vector<double>& constants)
 : _program(program), _registers(registerLength), _constants(constants), _constantsLength(constants.size()), _counter(0) {}

void GAsmInterpreter::setProgram(std::vector<uint8_t> &program) {
    _program = program;
}

//static void compile(std::vector<uint8_t> program) {
////    __asm(
////        "push rbp" // begin function, rbp is the stack
////        "mov  rbp, rsp" // rsp is the stack pointer
////        "mov  QWORD PTR [rbp-40], rdi" // rdi is the 1st argument (program) -40 cause we want it to be at the end (depends on the rest of the stack)
////        "mov  QWORD PTR [rbp-48], rsi" // rsi is 2nd argument (length) -48 last on stack, +8 from the size(double*)
////        "call constant()"
////    );
//    // TODO don't write it here but to the external function
//
//    __asm(
//        "push    rbp"
//        "mov     rbp, rsp"
//        "mov     QWORD PTR [rbp-40], rdi"
//        "mov     QWORD PTR [rbp-48], rsi"
//        "mov     QWORD PTR [rbp-56], rdx"
//        "mov     QWORD PTR [rbp-64], rcx"
//        "mov     QWORD PTR [rbp-72], r8"
//        "mov     QWORD PTR [rbp-80], r9"
//        "mov     QWORD PTR [rbp-8], 1000"
//        "mov     QWORD PTR [rbp-16], 0"
//        "mov     QWORD PTR [rbp-24], 0"
//        "pxor    xmm0, xmm0"
//        "movsd   QWORD PTR [rbp-32], xmm0"
//    );
//    for (size_t i = 0; i < program.size(); i++) {
//        const uint8_t &opcode = program[i];
//        switch (opcode) {
//
//            // ===== MOV =====
//            case MOV_P_A:  // P = A
////                P = static_cast<int>(A);
//                __asm("");
//                break;
//
//            case MOV_A_P:  // A = P
////                A = static_cast<double>(P);
//                __asm("");
//                break;
//
//            case MOV_A_R:  // A = R[P]
////                A = _registers[P % registerLength];
//                __asm("");
//                break;
//
//            case MOV_A_I:  // A = I[P]
////                A = inputs[P % inputLength];
//                __asm("");
//                break;
//
//            case MOV_R_A:  // R = A
////                _registers[P % registerLength] = A;
//                __asm("");
//                break;
//
//            case MOV_I_A:  // I[P] = A
////                inputs[P % inputLength] = A;
//                __asm("");
//                break;
//        }
//    }
//}
/*
     * rax and rdx are heavily used around in division
     * rdi - 1st argument; inputs pointer
     * rsi - 2nd argument; input length
     * rdx - 3rd argument; registers pointer
     * rcx - 4th argument; registers length
     * r8 - P; pointer
     * xmm0 - A; Accumulator
     */
/*
 * r8 - free to use
 * r9 - free to use
 * ;xmm0 is the Accumulator
 * pxor xmm0, xmm0; clear out the Accumulator
 * if the register length is a power of 2:
mov     rax, QWORD PTR [rbp-8]          ; load P
and     rax, QWORD PTR [rbp-64] - 1     ; mask = registerLength - 1
mov     rdx, QWORD PTR [rbp-56]         ; load registers base
movsd   xmm0, QWORD PTR [rbp-16]        ; load A
movsd   QWORD PTR [rdx + rax*8], xmm0   ; store A

 *
 * P = static_cast<int>(A);    ->
 * A = static_cast<double>(P); -> mov rax, QWORD PTR [rbp - STACK]; cvtsi2sd xmm0, rax; mov QWORD PTR [rbp
 * A = _registers[P % registerLength]; ->
 * A = inputs[P % inputLength]; ->
 * _registers[P % registerLength] = A; ->
 * inputs[P % inputLength] = A; ->
 */

//void run(double* inputs, size_t inputLength, double* registers, size_t registerLength, double* constants, size_t constantLength, double (*rng)(void)) {
//    // functions:
//    // constants() - gets new constant
//    // rand() - gets random from 0 to 1
//    constexpr size_t maxProcessTime = 1000;
//    size_t processTime = 0;
//    size_t P = 0;          // Program pointer
//    double A = 0;          // Accumulator
//    A = rng();
////    P = static_cast<int>(A);
////    A = static_cast<double>(P);
//    registers[P % registerLength] = A;
//    P++;
//    A = static_cast<double>(P);
//    registers[P % registerLength] = A;
//    while (A <= inputs[P % inputLength]) {
//        P--;
//        A = registers[P % registerLength];
//        P++;
//        A += registers[P % registerLength];
//        P++;
//        registers[P % registerLength] = A;
//        P = static_cast<size_t>(A);
//    }
//    A = registers[P % registerLength];
//    inputs[P % inputLength] = A;
////    MOV A, R // retrieve previous value
////    INC
////    ADD R    // add next value
////    INC
////    MOV R, A // save next value
////    MOV A, P // move count in P to A for loop to work
////    END
////    MOV A, R
////    MOV I, A // move last number to input (output)
//}

size_t GAsmInterpreter::run(std::vector<double> &inputs, size_t maxProcessTime) {
    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<double> dist(0, 1);

    _counter = 0; // reset constant counter
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

    for (size_t i = 0; i < _program.size(); i++) {
        const uint8_t& opcode = _program[i];
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
            case SET: A = _constants[_counter++ % _constantsLength]; break; // TODO func


            // ===== LOOPS =====
            case FOR:
                P = 0;   // Start loop at index 0
                pointerStack.push_back(i);  // push_back instruction
                instructionStack.push_back(FOR); // push_back current loop version
                pStack.push_back(P);        // save pointer
                break;

            case LOP_A:
                if (A <= inputs[P % inputLength]) {  // this has to be changed in 2 places
                    instructionStack.push_back(LOP_A);
                    pointerStack.push_back(i);
                } else {
                    // skip till the END symbol
                    skipToEnd = true;
                }
                break;

            case LOP_P:
                if (P <= inputLength) {  // this has to be changed in 2 places
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
                            if (A <= inputs[P % inputLength]) {  // this has to be changed in 2 places
                                i = pointerStack.back();
                            } else {
                                instructionStack.pop_back();
                                pointerStack.pop_back();
                            }
                            break;
                        case LOP_P:
                            if (P <= inputLength) {  // this has to be changed in 2 places
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
                A = static_cast<double>(dist(engine));
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
            for (int endCounter = 0; i < _program.size(); i++) {
                const uint8_t &instruction = _program[i];
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

