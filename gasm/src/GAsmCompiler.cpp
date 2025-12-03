
#include <vector>
#include <cmath>
#include <xbyak.h>
#include <memory>
#include "GAsmInterpreter.h"
#include "GAsmParser.h"

run_fn_t GAsmInterpreter::compile() {
    using namespace Xbyak::util;

    // used for nested loops and ifs
    auto endLabelStack = std::vector<Xbyak::Label>();
    auto startLabelStack = std::vector<Xbyak::Label>();
    auto instructionStack = std::vector<uint8_t>();
    size_t stackSize = 0;
    constexpr size_t spaceBetweenProcessTime = 10;
    size_t processTimeCounter = 0;

    // functions that will be called
    double (*exp_fn)(double) = exp;
    double (*sin_fn)(double) = sin;
    double (*cos_fn)(double) = cos;
    auto exp_asm = (uint64_t)(void*) exp_fn;
    auto sin_asm = (uint64_t)(void*) sin_fn;
    auto cos_asm = (uint64_t)(void*) cos_fn;

    // Important! don't use for constants:
    // rax - used in division and general operations
    // rdx - used in division

    // Register entry mapping (Intel 64, x86-64, AMD64)
    // 1: rdi -> inputs (double*)
    // 2: rsi -> inputLength (size_t)
    // 3: rdx -> registers (double*)
    // 4: rcx -> registerLength (size_t)
    // 5: r8  -> constants function<double()>
    // 6: r9  -> rng function<double()>
    //
    // Register mapping used in by the program:
    // r12 -> saved P % inputLength (size_t)
    #define PI r12
    // r13 -> saved P % registerLength (size_t)
    #define PR r13
    // QWORD PTR [rbp - 40] -> constants function<double()>
    #define constants qword[rbp - 40]
    // QWORD PTR [rbp - 48] -> rng function<double()>
    #define rng qword[rbp - 48]
    // rrbx -> P (size_t)
    #define P rbx
    // r13 -> saved inputs pointer (double*)
    #define inputs r14
    // QWORD PTR [rbp - 56] -> saved input length (size_t)
    #define inputLength qword[rbp - 56]
    // r14 -> saved registers pointer (double*)
    #define registers r15
    // QWORD PTR [rbp - 64] -> saved registers length (size_t)
    #define registerLength qword[rbp - 64]
    // QWORD PTR [rbp - 72] -> process time (size_t)
    #define processTime qword[rbp - 72]
    // QWORD PTR [rbp - 80] -> max process time
    #define maxProcessTime qword[rbp - 80]
    // dynamicStackSize -> the program could end in a for loop
    // so we have to keep track of the stack size
    // to later pop the correct amount
    #define dynamicStackSize qword[rbp - 88]
    // xmm0 -> A (accumulator) (double)
    #define A xmm0
    // we push 5 registers, so the stack starts at 40
    // then we have variables till 88,
    // but we add 8 to keep the stack aligned to 16 bytes
    #define LOCALS 96
    //
    // Registers free to use in the program
    // xmm1-15 - for floating numbers
    // rax, rdx, rcx

    // --- SETUP ---
    // push new frame pointer
    _code.push(rbp);
    // create new stack pointer
    _code.mov(rbp, rsp);

    // save caller's rbx, r12-r15, which we'll use
    // this changes the stack we have to be careful
    _code.push(rbx);
    _code.push(r12);
    _code.push(r13);
    _code.push(r14);
    _code.push(r15);

    // reserve beginning of the stack for our variables
    // WARNING!!! change if adding more locals
    _code.sub(rsp, LOCALS); // reserve stack of locals

    // move function arguments to appropriate registers
#if defined(__unix__)
    _code.mov(inputs, rdi);
    _code.mov(inputLength, rsi);
    _code.mov(registers, rdx);
    _code.mov(registerLength, rcx);
    _code.mov(constants, r8);  // constants
    _code.mov(rng, r9);        // rng;
    _code.mov(rax, qword[rbp + 8]);
    _code.mov(maxProcessTime, rax); // max process time;
#elif defined(_WIN64)
    _code.mov(inputs, rcx);
    _code.mov(inputLength, rdx);
    _code.mov(registers, r8);
    _code.mov(registerLength, r9);
    _code.mov(rax, qword[rbp + 48]);
    _code.mov(constants, rax);      // constants
    _code.mov(rax, qword[rbp + 56]);
    _code.mov(rng, rax);            // rng;
    _code.mov(rax, qword[rbp + 64]);
    _code.mov(maxProcessTime, rax); // max process time;
#else
#   error "Unsupported platform / calling convention"
#endif

    // reset P, PI, PR, A and processTime
    _code.xor_(P, P);            // P = 0
    _code.pxor(A, A);            // A = 0.0
    _code.xor_(PI, PI);          // PI = 0
    _code.xor_(PR, PR);          // PR = 0
    _code.mov(processTime, 0); // processTime = 0;
    _code.mov(dynamicStackSize, 0); // dynamicStackSize = 0;

    // prepare end program label
    Xbyak::Label endProgram;
    // TODO validate jump conditions
    // TODO simple optimization
    // TODO reuse registers and lea
    // TODO check process time every 10 instructions and on loops

    // --- COMPILATION ---
    for (int i = 0; i < _program->size(); i++) {
        const uint8_t& opcode = _program->operator[](i);

        switch (opcode) {
            case MOV_P_A: {
                // p = (size_t) A;
                // honestly I don't know why is it like this
                // it's copied from a C++ compiler
                _code.movsd(xmm1, A); // save A to xmm1
                _code.pxor(xmm2, xmm2);  // set xmm2 to 0
                _code.comisd(xmm1, xmm2); // compare xmm1 < xmm2, A < 0
                Xbyak::Label lessThan0;
                _code.jnb(lessThan0); // if A < 0: goto handle negative
                // case: A is positive, A >= 0
                _code.cvttsd2si(P, A); // convert A to size_t and save to P
                Xbyak::Label endConversion;
                _code.jmp(endConversion); // goto end
                // case: A is negative, A < 0
                _code.L(lessThan0);
                _code.subsd(A, xmm2);  // A = A - 0, it's to set the flags
                _code.cvttsd2si(P, A); // convert A to size_t and save to P
                _code.mov(rax, 0x8000000000000000); // bit magic
                _code.xor_(P, rax);    // make it non negative
                // end conversion
                _code.L(endConversion);
                // update P % registerLength
                _code.mov(rax, P);         // move P to rax
                _code.xor_(edx, edx);      // fill lower rdx with 0
                _code.div(registerLength); // division by length
                _code.mov(PR, rdx);        // remainder is in rdx
                // update P % inputLength
                _code.mov(rax, P);      // move P to rax
                _code.xor_(edx, edx);   // fill lower rdx with 0
                _code.div(inputLength); // division by length
                _code.mov(PI, rdx);     // remainder is in rdx
                break;
            }
            case MOV_A_P: {
                // A = (double) P
                _code.test(P, P);    // check if the value will fit in 64-bit number
                Xbyak::Label doesNotFit;
                _code.js(doesNotFit); // special case if the value won't fit
                // simple conversion the value will fit
                _code.pxor(A, A);     // clear A
                _code.cvtsi2sd(A, P); // convert A = (double) P
                Xbyak::Label endConversion;
                _code.jmp(endConversion);
                // complex conversion if the value won't fit
                // this is just some compiler magic and IEEE 754 standard
                _code.L(doesNotFit);
                _code.mov(rax, P);
                _code.mov(rdx, rax);
                _code.shr(rdx, 1);
                _code.and_(eax, 1);
                _code.or_(rdx, rax);
                _code.pxor(A, A);
                _code.cvtsi2sd(A, rdx);
                _code.addsd(A, A);
                // end conversion
                _code.L(endConversion);
                break;
            }
            case MOV_A_R: {
                // A = registers[P % registerLength]
                _code.movsd(A, ptr[registers + PR * 8]); // assign it to A
                break;
            }
            case MOV_A_I: {
                // A = inputs[P % inputLength]
                _code.movsd(A, ptr[inputs + PI * 8]); // assign it to A
                break;
            }
            case MOV_R_A: {
                // registers[P % registerLength] = A
                _code.movsd(ptr[registers + PR * 8], A); // assign A to it
                break;
            }
            case MOV_I_A: {
                // inputs[P % inputLength] = A
                _code.movsd(ptr[inputs + PI * 8], A); // assign A to it
                break;
            }
            case ADD_R: {
                // A += _registers[P % registerLength];
                _code.addsd(A, ptr[registers + PR * 8]); // add to A
                break;
            }
            case SUB_R: {
                // A -= _registers[P % registerLength];
                _code.subsd(A, ptr[registers + PR * 8]); // sub from A
                break;
            }
            case DIV_R: {
                // A /= _registers[P % registerLength];
                _code.divsd(A, ptr[registers + PR * 8]); // div A
                break;
            }
            case MUL_R: {
                // A *= _registers[P % registerLength];
                _code.mulsd(A, ptr[registers + PR * 8]); // mul with A
                break;
            }
            case SIN_R: {
                // A = sin(_registers[P % registerLength]);
                _code.movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                _code.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    _code.sub(rsp, 32);  // windows shadow space
#endif
                    _code.mov(rax, sin_asm);
                    _code.call(rax);     // call sin(xmm0), sin(A)
#ifdef _WIN64
                    _code.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    _code.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    _code.sub(rsp, 8);   // stack alignment
#endif
                    _code.mov(rax, sin_asm);
                    _code.call(rax);  // call sin(xmm0), sin(A)
#ifdef _WIN64
                    _code.add(rsp, 40);
#else
                    _code.sub(rsp, 8);
#endif
                }

                _code.pop(P); // restore the P register
                break;
            }
            case COS_R: {
                // A = cos(_registers[P % registerLength]);
                _code.movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                _code.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    _code.sub(rsp, 32);  // windows shadow space
#endif
                    _code.mov(rax, cos_asm);
                    _code.call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    _code.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    _code.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    _code.sub(rsp, 8);   // stack alignment
#endif
                    _code.mov(rax, cos_asm);
                    _code.call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    _code.add(rsp, 40);
#else
                    _code.sub(rsp, 8);
#endif
                }

                _code.pop(P); // restore the P register
                break;
            }
            case EXP_R: {
                // A = exp(_registers[P % registerLength]);
                _code.movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                _code.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    _code.sub(rsp, 32);  // windows shadow space
#endif
                    _code.mov(rax, exp_asm);
                    _code.call(rax);     // call exp(xmm0), exp(A
#ifdef _WIN64
                    _code.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    _code.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    _code.sub(rsp, 8);   // stack alignment
#endif
                    _code.mov(rax, exp_asm);
                    _code.call(rax);     // call exp(xmm0), exp(A
#ifdef _WIN64
                    _code.add(rsp, 40);
#else
                    _code.sub(rsp, 8);
#endif
                }

                _code.pop(P); // restore the P register
                break;
            }
            case ADD_I: {
                // A += _inputs[P % inputLength];
                _code.addsd(A, ptr[inputs + PI * 8]); // add to A
                break;
            }
            case SUB_I: {
                // A -= _inputs[P % inputLength];
                _code.subsd(A, ptr[inputs + PI * 8]); // sub from A
                break;
            }
            case DIV_I: {
                // A /= _inputs[P % inputLength];
                _code.divsd(A, ptr[inputs + PI * 8]); // div A
                break;
            }
            case MUL_I: {
                // A *= _inputs[P % inputLength];
                _code.mulsd(A, ptr[inputs + PI * 8]); // mul with A
                break;
            }
            case SIN_I: {
                // A = sin(_inputs[P % inputLength]);
                _code.movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                _code.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    _code.sub(rsp, 32);  // windows shadow space
#endif
                    _code.mov(rax, sin_asm);
                    _code.call(rax);     // call sin(xmm0), sin(A)
#ifdef _WIN64
                    _code.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    _code.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    _code.sub(rsp, 8);   // stack alignment
#endif
                    _code.mov(rax, sin_asm);
                    _code.call(rax);  // call sin(xmm0), sin(A)
#ifdef _WIN64
                    _code.add(rsp, 40);
#else
                    _code.sub(rsp, 8);
#endif
                }

                _code.pop(P); // restore the P register
                break;
            }
            case COS_I: {
                // A = cos(_inputs[P % inputLength]);
                _code.movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                _code.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    _code.sub(rsp, 32);  // windows shadow space
#endif
                    _code.mov(rax, cos_asm);
                    _code.call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    _code.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    _code.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    _code.sub(rsp, 8);   // stack alignment
#endif
                    _code.mov(rax, cos_asm);
                    _code.call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    _code.add(rsp, 40);
#else
                    _code.sub(rsp, 8);
#endif
                }

                _code.pop(P); // restore the P register
                break;
            }
            case EXP_I: {
                // A = exp(_inputs[P % inputLength]);
                _code.movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                _code.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    _code.sub(rsp, 32);  // windows shadow space
#endif
                    _code.mov(rax, exp_asm);
                    _code.call(rax);     // call exp(xmm0), exp(A)
#ifdef _WIN64
                    _code.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    _code.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    _code.sub(rsp, 8);   // stack alignment
#endif
                    _code.mov(rax, exp_asm);
                    _code.call(rax);     // call exp(xmm0), exp(A)
#ifdef _WIN64
                    _code.add(rsp, 40);
#else
                    _code.sub(rsp, 8);
#endif
                }

                _code.pop(P); // restore the P register
                break;
            }
            case INC: {
                // P++;
                _code.add(P, 1); // add 1
                // prepare rax, because cmove does not support immediate addressing
                _code.xor_(rax, rax); // rax = 0
                // update PI
                _code.add(PI, 1); // add 1
                _code.cmp(PI, inputLength); // compare to length
                _code.cmove(PI, rax); // set to 0 if equal length
                // update PR
                _code.add(PR, 1); // add 1
                _code.cmp(PR, registerLength); // compare to length
                _code.cmove(PR, rax); // set to 0 if equal length
                break;
            }
            case DEC: {
                // P--;
                _code.sub(P, 1); // sub 1
                // update PI
                _code.cmp(PI, 0); // compare to 0
                _code.cmove(PI, inputLength); // set to length if equal 0
                _code.sub(PI, 1); // sub 1
                // update PR
                _code.cmp(PR, 0); // compare to 0
                _code.cmove(PR, registerLength); // set to length if equal 0
                _code.sub(PR, 1); // sub 1

                break;
            }
            case RES: {
                // P = 0;
                _code.xor_(P, P); // set to 0
                _code.xor_(PI, PI); // set to 0
                _code.xor_(PR, PR); // set to 0
                break;
            }
            case SET: {
                // A = _constants[_counter++ % _constantsLength];
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                _code.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    _code.sub(rsp, 32);  // windows shadow space
#endif
                    _code.mov(rax, constants);
                    _code.call(rax);     // call constants
#ifdef _WIN64
                    _code.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    _code.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    _code.sub(rsp, 8);   // stack alignment
#endif
                    _code.mov(rax, constants);
                    _code.call(rax);     // call constants
#ifdef _WIN64
                    _code.add(rsp, 40);
#else
                    _code.sub(rsp, 8);
#endif
                }

                _code.pop(P); // restore the P register
                break;
            }
            case FOR: {
                instructionStack.push_back(FOR);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);     // create end label
                Xbyak::Label start;
                startLabelStack.push_back(start); // create start label
                // prepare the for loop
                _code.push(P);           // save P to stack
                stackSize++;             // increment stack size
                _code.add(dynamicStackSize, 1);
                _code.xor_(P, P);        // P = 0
                // we assume the inputLength is >= 1
                // that means the for loop will execute al least once
                // we don't have to check the condition the first time
                // loop start
                _code.L(startLabelStack.back());
                // move process time forward
//                _code.mov(rax, processTime);        // make a copy in rax
//                _code.add(rax, processTimeCounter); // add the amount of instructions
//                _code.mov(processTime, rax);        // save to process time
//                _code.cmp(rax, maxProcessTime);
//                _code.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
//                processTimeCounter = 0;
                break;
            }
            case LOP_A: {
                instructionStack.push_back(LOP_A); // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);      // create end label
                Xbyak::Label start;
                startLabelStack.push_back(start);  // create start label
                // condition is at the end
                _code.jmp(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to the end
                // loop start
                _code.L(startLabelStack.back());
                // move process time forward
//                _code.mov(rax, processTime);        // make a copy in rax
//                _code.add(rax, processTimeCounter); // add the amount of instructions
//                _code.mov(processTime, rax);        // save to process time
//                _code.cmp(rax, maxProcessTime);
//                _code.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
//                processTimeCounter = 0;
                break;
            }
            case LOP_P: {
                instructionStack.push_back(LOP_P); // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);      // create end label
                Xbyak::Label start;
                startLabelStack.push_back(start);  // create start label
                // condition is at the end
                _code.jmp(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to the end
                // loop start
                _code.L(startLabelStack.back());
                // move process time forward
//                _code.mov(rax, processTime);        // make a copy in rax
//                _code.add(rax, processTimeCounter); // add the amount of instructions
//                _code.mov(processTime, rax);        // save to process time
//                _code.cmp(rax, maxProcessTime);
//                _code.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
//                processTimeCounter = 0;
                break;
            }
            case JMP_I: {
                instructionStack.push_back(JMP_I);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                _code.movsd(xmm1, ptr[inputs + PI*8]); // xmm1 = I[P % length]
                _code.comisd(A, xmm1);           // compare A and xmm1
                _code.jnb(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if A >= xmm1
                break;
            }
            case JMP_R: {
                instructionStack.push_back(JMP_R);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                _code.movsd(xmm1, ptr[registers + PR*8]); // xmm1 = R[P % length]
                _code.comisd(A, xmm1);           // compare A and xmm1
                _code.jnb(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if A >= xmm1
                break;
            }
            case JMP_P: {
                instructionStack.push_back(JMP_P);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                _code.cvtsi2sd(xmm1, P);         // xmm1 = (double) P
                _code.comisd(xmm1, A);           // compare xmm1 and A
                _code.jnb(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if xmm1 >= A
                break;
            }
            case END: {
                // assign label if there is one, else do nothing
                if (!endLabelStack.empty()) {
                    switch (instructionStack.back()) {
                        case FOR: {
                            _code.L(endLabelStack.back()); // bind the end label
                            _code.add(P, 1);               // increment P
                            _code.cmp(P, inputLength);     // compare with length
                            _code.jnge(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if P < length
                            // end loop
                            _code.pop(P);            // restore P
                            stackSize--;             // decrement stack size
                            _code.sub(dynamicStackSize, 1);
                            // pop all the stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                            startLabelStack.pop_back();
                            break;
                        }
                        case LOP_A: {
                            _code.L(endLabelStack.back());     // bind the end label
                            _code.movsd(xmm1, ptr[inputs + PI*8]); // xmm1 = I[P % length]
                            _code.comisd(A, xmm1);             // compare A and xmm1
                            _code.jg(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if A < xmm1
                            // end loop, pop all the stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                            startLabelStack.pop_back();
                            break;
                        }
                        case LOP_P: {
                            _code.L(endLabelStack.back());     // bind the end label
                            _code.mov(rax, P);                 // save P to rax
                            _code.cmp(rax, inputLength);       // compare P and input length
                            _code.jg(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if P < inputLength
                            // end loop, pop all the stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                            startLabelStack.pop_back();
                            break;
                        }
                        default: {
                            // this case is for JMP_I, JMP_R, JMP_P
                            _code.L(endLabelStack.back()); // bind the end label
                            // pop end and instruction stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                        }
                    }
                }
                break;
            }
            case RNG: {
                // A = rng();
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                _code.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    _code.sub(rsp, 32);  // windows shadow space
#endif
                    _code.mov(rax, rng);
                    _code.call(rax);     // call rng
#ifdef _WIN64
                    _code.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    _code.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    _code.sub(rsp, 8);   // stack alignment
#endif
                    _code.mov(rax, rng);
                    _code.call(rax);     // call rng
#ifdef _WIN64
                    _code.add(rsp, 40);
#else
                    _code.sub(rsp, 8);
#endif
                }

                _code.pop(P); // restore the P register
            }
            default: {
                break;
            }
        }
        // increase process time and check if it's the end
        _code.mov(rax, processTime); // make a copy in rax
        _code.add(rax, 1);           // add 1
        _code.mov(processTime, rax); // save to process time
        _code.cmp(rax, maxProcessTime);
        _code.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
        // TODO make it faster, do it every 10 times and on loop entry TESTING
//        processTimeCounter++;
//        if (processTimeCounter == spaceBetweenProcessTime) {
//            _code.mov(rax, processTime);             // make a copy in rax
//            _code.add(rax, spaceBetweenProcessTime); // add step amount
//            _code.mov(processTime, rax);             // save to process time
//            _code.cmp(rax, maxProcessTime);
//            _code.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
//            processTimeCounter = 0;
//        }
    }
    // put unused labels at the end
    while (!endLabelStack.empty()) {
        switch (instructionStack.back()) {
            case FOR: {
                _code.L(endLabelStack.back()); // bind the end label
                _code.add(P, 1);               // increment P
                _code.cmp(P, inputLength);     // compare with length
                _code.jnge(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if P < length
                // end loop
                _code.pop(P);            // restore P
                stackSize--;             // decrement stack size
                _code.sub(dynamicStackSize, 1);
                // pop all the stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
                startLabelStack.pop_back();
                break;
            }
            case LOP_A: {
                _code.L(endLabelStack.back());     // bind the end label
                _code.movsd(xmm1, ptr[inputs + PI*8]); // xmm1 = I[P % length]
                _code.comisd(A, xmm1);             // compare A and xmm1
                _code.jnle(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if A <= xmm1
                // end loop, pop all the stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
                startLabelStack.pop_back();
                break;
            }
            case LOP_P: {
                _code.L(endLabelStack.back());     // bind the end label
                _code.mov(rax, P);                 // save P to rax
                _code.cmp(rax, inputLength);       // compare P and input length
                _code.jng(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if P <= inputLength
                // end loop, pop all the stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
                startLabelStack.pop_back();
                break;
            }
            default: {
                // this case is for JMP_I, JMP_R, JMP_P
                _code.L(endLabelStack.back()); // bind the end label
                // pop end and instruction stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
            }
        }
    }
    // end of the program
    _code.L(endProgram);
    _code.mov(rax, dynamicStackSize); // prepare for stack restoration
    _code.shl(rax, 3);                // * 8, move stack by 8 for every push
    _code.add(rsp, rax);              // pop from stack
    _code.mov(rax, processTime); // return process time
//    _code.add(rax, spaceBetweenProcessTime - processTimeCounter - 1); // add remaining process time
    _code.add(rsp, LOCALS); // restore stack of locals
    // restore the caller's stack
    _code.pop(r15);
    _code.pop(r14);
    _code.pop(r13);
    _code.pop(r12);
    _code.pop(rbx);
    _code.pop(rbp);
    _code.ret();    // return from function

    // finalize and get function pointer
    _code.ready();
    return _code.getCode<run_fn_t>();
}
