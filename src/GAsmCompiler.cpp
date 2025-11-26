
#include <vector>
#include <cmath>
#include <xbyak.h>
#include <memory>
#include "GAsmInterpreter.h"
#include "GAsmParser.h"

run_fn_t GAsmInterpreter::compile() {
    using namespace Xbyak::util;

    struct Jit : Xbyak::CodeGenerator {
        // 4096 default program size
        // (void*) 1 - auto grow buffer
        Jit() : Xbyak::CodeGenerator(1, Xbyak::AutoGrow) {

        }
    };

    auto code = std::make_unique<Jit>();
    // used for nested loops and ifs
    auto endLabelStack = std::vector<Xbyak::Label>();
    auto startLabelStack = std::vector<Xbyak::Label>();
    auto instructionStack = std::vector<uint8_t>();
    size_t stackSize = 0;

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
    code->push(rbp);
    // create new stack pointer
    code->mov(rbp, rsp);

    // save caller's rbx, r12-r15, which we'll use
    // this changes the stack we have to be careful
    code->push(rbx);
    code->push(r12);
    code->push(r13);
    code->push(r14);
    code->push(r15);

    // reserve beginning of the stack for our variables
    // WARNING!!! change if adding more locals
    code->sub(rsp, LOCALS); // reserve stack of locals

    // move function arguments to appropriate registers
#ifdef __unix__
    code->mov(inputs, rdi);
    code->mov(inputLength, rsi);
    code->mov(registers, rdx);
    code->mov(registerLength, rcx);
    code->mov(constants, r8);  // constants
    code->mov(rng, r9);        // rng;
    code->mov(rax, qword[rbp + 8]);
    code->mov(maxProcessTime, rax); // max process time;
#elifdef _WIN64
    code->mov(inputs, rcx);
    code->mov(inputLength, rdx);
    code->mov(registers, r8);
    code->mov(registerLength, r9);
    code->mov(rax, qword[rbp + 48]);
    code->mov(constants, rax);      // constants
    code->mov(rax, qword[rbp + 56]);
    code->mov(rng, rax);            // rng;
    code->mov(rax, qword[rbp + 64]);
    code->mov(maxProcessTime, rax); // max process time;
#endif

    // reset P, PI, PR, A and processTime
    code->xor_(P, P);            // P = 0
    code->pxor(A, A);            // A = 0.0
    code->xor_(PI, PI);          // PI = 0
    code->xor_(PR, PR);          // PR = 0
    code->mov(processTime, 0); // processTime = 0;
    code->mov(dynamicStackSize, 0); // dynamicStackSize = 0;

    // prepare end program label
    Xbyak::Label endProgram;
    // TODO remove stackSize and just track it locally TEST
    // TODO add maxProcessTime as a parameter to function TEST
    // TODO validate jump conditions
    // TODO simple optimization
    // TODO reuse registers and lea
    // TODO check process time every 10 instructions and on loops
    // TODO T_FAR is not supported, but T_NEAR works for long jumps XD
    // --- COMPILATION ---
    for (int i = 0; i < _program.size(); i++) {
        const uint8_t& opcode = _program[i];

        switch (opcode) {
            case MOV_P_A: {
                // p = (size_t) A;
                // honestly I don't know why is it like this
                // it's copied from a C++ compiler
                code->movsd(xmm1, A); // save A to xmm1
                code->pxor(xmm2, xmm2);  // set xmm2 to 0
                code->comisd(xmm1, xmm2); // compare xmm1 < xmm2, A < 0
                Xbyak::Label lessThan0;
                code->jnb(lessThan0); // if A < 0: goto handle negative
                // case: A is positive, A >= 0
                code->cvttsd2si(P, A); // convert A to size_t and save to P
                Xbyak::Label endConversion;
                code->jmp(endConversion); // goto end
                // case: A is negative, A < 0
                code->L(lessThan0);
                code->subsd(A, xmm2);  // A = A - 0, it's to set the flags
                code->cvttsd2si(P, A); // convert A to size_t and save to P
                code->mov(rax, 0x8000000000000000); // bit magic
                code->xor_(P, rax);    // make it non negative
                // end conversion
                code->L(endConversion);
                // update P % registerLength
                code->mov(rax, P);         // move P to rax
                code->xor_(edx, edx);      // fill lower rdx with 0
                code->div(registerLength); // division by length
                code->mov(PR, rdx);        // remainder is in rdx
                // update P % inputLength
                code->mov(rax, P);      // move P to rax
                code->xor_(edx, edx);   // fill lower rdx with 0
                code->div(inputLength); // division by length
                code->mov(PI, rdx);     // remainder is in rdx
                break;
            }
            case MOV_A_P: {
                // A = (double) P
                code->test(P, P);    // check if the value will fit in 64-bit number
                Xbyak::Label doesNotFit;
                code->js(doesNotFit); // special case if the value won't fit
                // simple conversion the value will fit
                code->pxor(A, A);     // clear A
                code->cvtsi2sd(A, P); // convert A = (double) P
                Xbyak::Label endConversion;
                code->jmp(endConversion);
                // complex conversion if the value won't fit
                // this is just some compiler magic and IEEE 754 standard
                code->L(doesNotFit);
                code->mov(rax, P);
                code->mov(rdx, rax);
                code->shr(rdx, 1);
                code->and_(eax, 1);
                code->or_(rdx, rax);
                code->pxor(A, A);
                code->cvtsi2sd(A, rdx);
                code->addsd(A, A);
                // end conversion
                code->L(endConversion);
                break;
            }
            case MOV_A_R: {
                // A = registers[P % registerLength]
                code->movsd(A, ptr[registers + PR * 8]); // assign it to A
                break;
            }
            case MOV_A_I: {
                // A = inputs[P % inputLength]
                code->movsd(A, ptr[inputs + PI * 8]); // assign it to A
                break;
            }
            case MOV_R_A: {
                // registers[P % registerLength] = A
                code->movsd(ptr[registers + PR * 8], A); // assign A to it
                break;
            }
            case MOV_I_A: {
                // inputs[P % inputLength] = A
                code->movsd(ptr[inputs + PI * 8], A); // assign A to it
                break;
            }
            case ADD_R: {
                // A += _registers[P % registerLength];
                code->addsd(A, ptr[registers + PR * 8]); // add to A
                break;
            }
            case SUB_R: {
                // A -= _registers[P % registerLength];
                code->subsd(A, ptr[registers + PR * 8]); // sub from A
                break;
            }
            case DIV_R: {
                // A /= _registers[P % registerLength];
                code->divsd(A, ptr[registers + PR * 8]); // div A
                break;
            }
            case MUL_R: {
                // A *= _registers[P % registerLength];
                code->mulsd(A, ptr[registers + PR * 8]); // mul with A
                break;
            }
            case SIN_R: {
                // A = sin(_registers[P % registerLength]);
                code->movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code->sub(rsp, 32);  // windows shadow space
#endif
                    code->mov(rax, (size_t) sin);
                    code->call(rax);     // call sin(xmm0), sin(A)
#ifdef _WIN64
                    code->add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code->sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code->sub(rsp, 8);   // stack alignment
#endif
                    code->mov(rax, (size_t) sin);
                    code->call(rax);  // call sin(xmm0), sin(A)
#ifdef _WIN64
                    code->add(rsp, 40);
#else
                    code->sub(rsp, 8);
#endif
                }

                code->pop(P); // restore the P register
                break;
            }
            case COS_R: {
                // A = cos(_registers[P % registerLength]);
                code->movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code->sub(rsp, 32);  // windows shadow space
#endif
                    code->mov(rax, (size_t) cos);
                    code->call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    code->add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code->sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code->sub(rsp, 8);   // stack alignment
#endif
                    code->mov(rax, (size_t) cos);
                    code->call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    code->add(rsp, 40);
#else
                    code->sub(rsp, 8);
#endif
                }

                code->pop(P); // restore the P register
                break;
            }
            case EXP_R: {
                // A = exp(_registers[P % registerLength]);
                code->movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code->sub(rsp, 32);  // windows shadow space
#endif
                    code->mov(rax, (size_t)exp);
                    code->call(rax);     // call exp(xmm0), exp(A
#ifdef _WIN64
                    code->add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code->sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code->sub(rsp, 8);   // stack alignment
#endif
                    code->mov(rax, (size_t)exp);
                    code->call(rax);     // call exp(xmm0), exp(A
#ifdef _WIN64
                    code->add(rsp, 40);
#else
                    code->sub(rsp, 8);
#endif
                }

                code->pop(P); // restore the P register
                break;
            }
            case ADD_I: {
                // A += _inputs[P % inputLength];
                code->addsd(A, ptr[inputs + PI * 8]); // add to A
                break;
            }
            case SUB_I: {
                // A -= _inputs[P % inputLength];
                code->subsd(A, ptr[inputs + PI * 8]); // sub from A
                break;
            }
            case DIV_I: {
                // A /= _inputs[P % inputLength];
                code->divsd(A, ptr[inputs + PI * 8]); // div A
                break;
            }
            case MUL_I: {
                // A *= _inputs[P % inputLength];
                code->mulsd(A, ptr[inputs + PI * 8]); // mul with A
                break;
            }
            case SIN_I: {
                // A = sin(_inputs[P % inputLength]);
                code->movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code->sub(rsp, 32);  // windows shadow space
#endif
                    code->mov(rax, (size_t) sin);
                    code->call(rax);     // call sin(xmm0), sin(A)
#ifdef _WIN64
                    code->add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code->sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code->sub(rsp, 8);   // stack alignment
#endif
                    code->mov(rax, (size_t) sin);
                    code->call(rax);  // call sin(xmm0), sin(A)
#ifdef _WIN64
                    code->add(rsp, 40);
#else
                    code->sub(rsp, 8);
#endif
                }

                code->pop(P); // restore the P register
                break;
            }
            case COS_I: {
                // A = cos(_inputs[P % inputLength]);
                code->movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code->sub(rsp, 32);  // windows shadow space
#endif
                    code->mov(rax, (size_t) cos);
                    code->call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    code->add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code->sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code->sub(rsp, 8);   // stack alignment
#endif
                    code->mov(rax, (size_t) cos);
                    code->call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    code->add(rsp, 40);
#else
                    code->sub(rsp, 8);
#endif
                }

                code->pop(P); // restore the P register
                break;
            }
            case EXP_I: {
                // A = exp(_inputs[P % inputLength]);
                code->movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code->sub(rsp, 32);  // windows shadow space
#endif
                    code->mov(rax, (size_t)exp);
                    code->call(rax);     // call exp(xmm0), exp(A
#ifdef _WIN64
                    code->add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code->sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code->sub(rsp, 8);   // stack alignment
#endif
                    code->mov(rax, (size_t) exp);
                    code->call(rax);     // call exp(xmm0), exp(A
#ifdef _WIN64
                    code->add(rsp, 40);
#else
                    code->sub(rsp, 8);
#endif
                }

                code->pop(P); // restore the P register
                break;
            }
            case INC: {
                // P++;
                code->add(P, 1); // add 1
                // prepare rax, because cmove does not support immediate addressing
                code->xor_(rax, rax); // rax = 0
                // update PI
                code->add(PI, 1); // add 1
                code->cmp(PI, inputLength); // compare to length
                code->cmove(PI, rax); // set to 0 if equal length
                // update PR
                code->add(PR, 1); // add 1
                code->cmp(PR, registerLength); // compare to length
                code->cmove(PR, rax); // set to 0 if equal length
                break;
            }
            case DEC: {
                // P--;
                code->sub(P, 1); // sub 1
                // update PI
                code->cmp(PI, 0); // compare to 0
                code->cmove(PI, inputLength); // set to length if equal 0
                code->sub(PI, 1); // sub 1
                // update PR
                code->cmp(PR, 0); // compare to 0
                code->cmove(PR, registerLength); // set to length if equal 0
                code->sub(PR, 1); // sub 1

                break;
            }
            case RES: {
                // P = 0;
                code->xor_(P, P); // set to 0
                code->xor_(PI, PI); // set to 0
                code->xor_(PR, PR); // set to 0
                break;
            }
            case SET: {
                // A = _constants[_counter++ % _constantsLength];
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code->sub(rsp, 32);  // windows shadow space
#endif
                    code->mov(rax, constants);
                    code->call(rax);     // call constants
#ifdef _WIN64
                    code->add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code->sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code->sub(rsp, 8);   // stack alignment
#endif
                    code->mov(rax, constants);
                    code->call(rax);     // call constants
#ifdef _WIN64
                    code->add(rsp, 40);
#else
                    code->sub(rsp, 8);
#endif
                }

                code->pop(P); // restore the P register
                break;
            }
            case FOR: {
                instructionStack.push_back(FOR);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);     // create end label
                Xbyak::Label start;
                startLabelStack.push_back(start); // create start label
                // prepare the for loop
                code->push(P);           // save P to stack
                stackSize++;             // increment stack size
                code->add(dynamicStackSize, 1);
                code->xor_(P, P);        // P = 0
                // we assume the inputLength is >= 1
                // that means the for loop will execute al least once
                // we don't have to check the condition the first time
                // loop start
                code->L(startLabelStack.back());
                break;
            }
            case LOP_A: {
                instructionStack.push_back(LOP_A); // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);      // create end label
                Xbyak::Label start;
                startLabelStack.push_back(start);  // create start label
                // condition is at the end
                code->jmp(endLabelStack.back(), Jit::T_NEAR); // long jump to the end
                // loop start
                code->L(startLabelStack.back());
                break;
            }
            case LOP_P: {
                instructionStack.push_back(LOP_P); // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);      // create end label
                Xbyak::Label start;
                startLabelStack.push_back(start);  // create start label
                // condition is at the end
                code->jmp(endLabelStack.back(), Jit::T_NEAR); // long jump to the end
                // loop start
                code->L(startLabelStack.back());
                break;
            }
            case JMP_I: {
                instructionStack.push_back(JMP_I);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                code->movsd(xmm1, ptr[inputs + PI*8]); // xmm1 = I[P % length]
                code->comisd(A, xmm1);           // compare A and xmm1
                code->jnb(endLabelStack.back(), Jit::T_NEAR); // long jump if A >= xmm1
                break;
            }
            case JMP_R: {
                instructionStack.push_back(JMP_R);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                code->movsd(xmm1, ptr[registers + PR*8]); // xmm1 = R[P % length]
                code->comisd(A, xmm1);           // compare A and xmm1
                code->jnb(endLabelStack.back(), Jit::T_NEAR); // long jump if A >= xmm1
                break;
            }
            case JMP_P: {
                instructionStack.push_back(JMP_P);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                code->cvtsi2sd(xmm1, P);         // xmm1 = (double) P
                code->comisd(xmm1, A);           // compare xmm1 and A
                code->jnb(endLabelStack.back(), Jit::T_NEAR); // long jump if xmm1 >= A
                break;
            }
            case END: {
                // assign label if there is one, else do nothing
                if (!endLabelStack.empty()) {
                    switch (instructionStack.back()) {
                        case FOR: {
                            code->L(endLabelStack.back()); // bind the end label
                            code->add(P, 1);               // increment P
                            code->cmp(P, inputLength);     // compare with length
                            code->jnge(startLabelStack.back(), Jit::T_NEAR); // long jump if P < length
                            // end loop
                            code->pop(P);            // restore P
                            stackSize--;             // decrement stack size
                            code->sub(dynamicStackSize, 1);
                            // pop all the stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                            startLabelStack.pop_back();
                            break;
                        }
                        case LOP_A: {
                            code->L(endLabelStack.back());     // bind the end label
                            code->movsd(xmm1, ptr[inputs + PI*8]); // xmm1 = I[P % length]
                            code->comisd(A, xmm1);             // compare A and xmm1
                            code->jnle(startLabelStack.back(), Jit::T_NEAR); // long jump if A <= xmm1
                            // end loop, pop all the stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                            startLabelStack.pop_back();
                            break;
                        }
                        case LOP_P: {
                            code->L(endLabelStack.back());     // bind the end label
                            code->mov(rax, P);                 // save P to rax
                            code->cmp(rax, inputLength);       // compare P and input length
                            code->jng(startLabelStack.back(), Jit::T_NEAR); // long jump if P <= inputLength
                            // end loop, pop all the stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                            startLabelStack.pop_back();
                            break;
                        }
                        default: {
                            // this case is for JMP_I, JMP_R, JMP_P
                            code->L(endLabelStack.back()); // bind the end label
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
                code->push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code->sub(rsp, 32);  // windows shadow space
#endif
                    code->mov(rax, rng);
                    code->call(rax);     // call rng
#ifdef _WIN64
                    code->add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code->sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code->sub(rsp, 8);   // stack alignment
#endif
                    code->mov(rax, rng);
                    code->call(rax);     // call rng
#ifdef _WIN64
                    code->add(rsp, 40);
#else
                    code->sub(rsp, 8);
#endif
                }

                code->pop(P); // restore the P register
            }
            default: {
                break;
            }
        }
        // increase process time and check if it's the end
        code->mov(rax, processTime); // make a copy in rax
        code->add(rax, 1);           // add 1
        code->mov(processTime, rax); // save to process time
        code->cmp(rax, maxProcessTime);
        code->ja(endProgram, Jit::T_NEAR); // long jump to end
    }
    // put unused labels at the end
    while (!endLabelStack.empty()) {
        switch (instructionStack.back()) {
            case FOR: {
                code->L(endLabelStack.back()); // bind the end label
                code->add(P, 1);               // increment P
                code->cmp(P, inputLength);     // compare with length
                code->jnge(startLabelStack.back(), Jit::T_NEAR); // long jump if P < length
                // end loop
                code->pop(P);            // restore P
                stackSize--;             // decrement stack size
                code->sub(dynamicStackSize, 1);
                // pop all the stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
                startLabelStack.pop_back();
                break;
            }
            case LOP_A: {
                code->L(endLabelStack.back());     // bind the end label
                code->movsd(xmm1, ptr[inputs + PI*8]); // xmm1 = I[P % length]
                code->comisd(A, xmm1);             // compare A and xmm1
                code->jnle(startLabelStack.back(), Jit::T_NEAR); // long jump if A <= xmm1
                // end loop, pop all the stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
                startLabelStack.pop_back();
                break;
            }
            case LOP_P: {
                code->L(endLabelStack.back());     // bind the end label
                code->mov(rax, P);                 // save P to rax
                code->cmp(rax, inputLength);       // compare P and input length
                code->jng(startLabelStack.back(), Jit::T_NEAR); // long jump if P <= inputLength
                // end loop, pop all the stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
                startLabelStack.pop_back();
                break;
            }
            default: {
                // this case is for JMP_I, JMP_R, JMP_P
                code->L(endLabelStack.back()); // bind the end label
                // pop end and instruction stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
            }
        }
    }
    // end of the program
    code->L(endProgram);
    code->mov(rax, dynamicStackSize); // prepare for stack restoration
    code->shl(rax, 3);                // * 8, move stack by 8 for every push
    code->add(rsp, rax);              // pop from stack
    code->mov(rax, processTime); // return process time
    code->add(rsp, LOCALS); // restore stack of locals
    // restore the caller's stack
    code->pop(r15);
    code->pop(r14);
    code->pop(r13);
    code->pop(r12);
    code->pop(rbx);
    code->pop(rbp);
    code->ret();    // return from function

    // finalize and get function pointer
    code->ready();
    auto programFunction = code->getCode<run_fn_t>();
    // TODO return
    // transfer ownership - keep code alive (must not free while function used)
    // We'll leak code pointer here for simplicity; in production store unique_ptr<Jit> somewhere.
    // For demo we return the pointer and deliberately release ownership, but keep `code` alive by releasing pointer.
    // To keep it simple: release unique_ptr so code memory stays allocated until program exit.
    // Note: xbyak uses mmap internally and will free on destructor; to keep code executable, we release ownership.
    code.release();
    return programFunction;
}
