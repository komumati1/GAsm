
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

    // Register mapping used in by the program:
    // r12 -> saved P % inputLength (size_t)
    #define PI r12
    // r13 -> saved P % registerLength (size_t)
    #define PR r13
    // QWORD PTR [rbp - 48] -> constants function<double()>
    #define constants qword[rbp - 48]
    // QWORD PTR [rbp - 56] -> rng function<double()>
    #define rng qword[rbp - 56]
    // rbx -> P (size_t)
    #define P rbx
    // r14 -> saved inputs pointer (double*)
    #define inputs r14
    // QWORD PTR [rbp - 64] -> saved input length (size_t)
    #define inputLength qword[rbp - 64]
    // r15 -> saved registers pointer (double*)
    #define registers r15
    // QWORD PTR [rbp - 72] -> saved registers length (size_t)
    #define registerLength qword[rbp - 72]
    // QWORD PTR [rbp - 80] -> process time (size_t)
    #define processTime qword[rbp - 80]
    // QWORD PTR [rbp - 88] -> max process time
    #define maxProcessTime qword[rbp - 88]
    // dynamicStackSize -> the program could end in a for loop
    // so we have to keep track of the stack size
    // to later pop the correct amount
    #define dynamicStackSize qword[rbp - 96]
    // xmm0 -> A (accumulator) (double)
    #define A xmm0
    // we push 5 registers, so the stack starts at 40
    // then we have variables till 88,
    // but we add 8 to keep the stack aligned to 16 bytes
    #define LOCALS 96
    //
    // Registers free to use in the program
    // xmm1-5 - for floating numbers
    // rax, rdx, rcx

    // --- SETUP ---
    // push new frame pointer
    code_.push(rbp);
    // create new stack pointer
    code_.mov(rbp, rsp);

    // save caller's rbx, r12-r15, which we'll use
    // this changes the stack we have to be careful
    code_.push(rbx);
    code_.push(r12);
    code_.push(r13);
    code_.push(r14);
    code_.push(r15);

    // reserve beginning of the stack for our variables
    // WARNING!!! change if adding more locals
    code_.sub(rsp, LOCALS); // reserve stack of locals

    // move function arguments to appropriate registers
#if defined(__unix__)
// System V AMD64 ABI:
// 1: inputs          -> rdi
// 2: inputLength     -> rsi
// 3: registers       -> rdx
// 4: registerLength  -> rcx
// 5: constants       -> r8
// 6: rng             -> r9
// 7: maxProcessTime  -> [rsp+8] at entry -> [rbp+16] after push rbp/mov rbp,rsp
    code_.mov(inputs, rdi);
    code_.mov(inputLength, rsi);
    code_.mov(registers, rdx);
    code_.mov(registerLength, rcx);
    code_.mov(constants, r8);  // constants
    code_.mov(rng, r9);        // rng;
    code_.mov(rax, qword[rbp + 16]);
    code_.mov(maxProcessTime, rax); // max process time;
#elif defined(_WIN64)
// Microsoft x64 ABI:
// 1: inputs          -> rcx
// 2: inputLength     -> rdx
// 3: registers       -> r8
// 4: registerLength  -> r9
// 5: constants       -> [rsp+40] at entry
// 6: rng             -> [rsp+48]
// 7: maxProcessTime  -> [rsp+56]
// After push rbp/mov rbp,rsp those become +48, +56, +64 respectively.
    code_.mov(inputs, rcx);
    code_.mov(inputLength, rdx);
    code_.mov(registers, r8);
    code_.mov(registerLength, r9);
    code_.mov(rax, qword[rbp + 48]);
    code_.mov(constants, rax);      // constants
    code_.mov(rax, qword[rbp + 56]);
    code_.mov(rng, rax);            // rng;
    code_.mov(rax, qword[rbp + 64]);
    code_.mov(maxProcessTime, rax); // max process time;
#else
#   error "Unsupported platform / calling convention"
#endif

    // reset P, PI, PR, A and processTime
    code_.xor_(P, P);            // P = 0
    code_.pxor(A, A);            // A = 0.0
    code_.xor_(PI, PI);          // PI = 0
    code_.xor_(PR, PR);          // PR = 0
    code_.mov(processTime, 0); // processTime = 0;
    code_.mov(dynamicStackSize, 0); // dynamicStackSize = 0;

    // prepare end program label
    Xbyak::Label endProgram;
    // TODO validate jump conditions
    // TODO simple optimization
    // TODO reuse registers and lea
    // TODO check process time every 10 instructions and on loops

    // --- COMPILATION ---
    for (int i = 0; i < program_->size(); i++) {
        const uint8_t& opcode = program_->operator[](i);

        switch (opcode) {
            case MOV_P_A: {
                // p = (size_t) A;
                // honestly I don't know why is it like this
                // it's copied from a C++ compiler
                code_.movsd(xmm1, A); // save A to xmm1
                code_.pxor(xmm2, xmm2);  // set xmm2 to 0
                code_.comisd(xmm1, xmm2); // compare xmm1 < xmm2, A < 0
                Xbyak::Label lessThan0;
                code_.jnb(lessThan0); // if A < 0: goto handle negative
                // case: A is positive, A >= 0
                code_.cvttsd2si(P, A); // convert A to size_t and save to P
                Xbyak::Label endConversion;
                code_.jmp(endConversion); // goto end
                // case: A is negative, A < 0
                code_.L(lessThan0);
                code_.subsd(A, xmm2);  // A = A - 0, it's to set the flags
                code_.cvttsd2si(P, A); // convert A to size_t and save to P
                code_.mov(rax, 0x8000000000000000); // bit magic
                code_.xor_(P, rax);    // make it non negative
                // end conversion
                code_.L(endConversion);
                // update P % registerLength
                code_.mov(rax, P);         // move P to rax
                code_.xor_(edx, edx);      // fill lower rdx with 0
                code_.div(registerLength); // division by length
                code_.mov(PR, rdx);        // remainder is in rdx
                // update P % inputLength
                code_.mov(rax, P);      // move P to rax
                code_.xor_(edx, edx);   // fill lower rdx with 0
                code_.div(inputLength); // division by length
                code_.mov(PI, rdx);     // remainder is in rdx
                break;
            }
            case MOV_A_P: {
                // A = (double) P
                code_.test(P, P);    // check if the value will fit in 64-bit number
                Xbyak::Label doesNotFit;
                code_.js(doesNotFit); // special case if the value won't fit
                // simple conversion the value will fit
                code_.pxor(A, A);     // clear A
                code_.cvtsi2sd(A, P); // convert A = (double) P
                Xbyak::Label endConversion;
                code_.jmp(endConversion);
                // complex conversion if the value won't fit
                // this is just some compiler magic and IEEE 754 standard
                code_.L(doesNotFit);
                code_.mov(rax, P);
                code_.mov(rdx, rax);
                code_.shr(rdx, 1);
                code_.and_(eax, 1);
                code_.or_(rdx, rax);
                code_.pxor(A, A);
                code_.cvtsi2sd(A, rdx);
                code_.addsd(A, A);
                // end conversion
                code_.L(endConversion);
                break;
            }
            case MOV_A_R: {
                // A = registers[P % registerLength]
                code_.movsd(A, ptr[registers + PR * 8]); // assign it to A
                break;
            }
            case MOV_A_I: {
                // A = inputs[P % inputLength]
                code_.movsd(A, ptr[inputs + PI * 8]); // assign it to A
                break;
            }
            case MOV_R_A: {
                // registers[P % registerLength] = A
                code_.movsd(ptr[registers + PR * 8], A); // assign A to it
                break;
            }
            case MOV_I_A: {
                // inputs[P % inputLength] = A
                code_.movsd(ptr[inputs + PI * 8], A); // assign A to it
                break;
            }
            case ADD_R: {
                // A += registers_[P % registerLength];
                code_.addsd(A, ptr[registers + PR * 8]); // add to A
                break;
            }
            case SUB_R: {
                // A -= registers_[P % registerLength];
                code_.subsd(A, ptr[registers + PR * 8]); // sub from A
                break;
            }
            case DIV_R: {
                // A /= registers_[P % registerLength];
                code_.divsd(A, ptr[registers + PR * 8]); // div A
                break;
            }
            case MUL_R: {
                // A *= registers_[P % registerLength];
                code_.mulsd(A, ptr[registers + PR * 8]); // mul with A
                break;
            }
            case SIN_R: {
                // A = sin(registers_[P % registerLength]);
                code_.movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code_.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code_.sub(rsp, 32);  // windows shadow space
#endif
                    code_.mov(rax, sin_asm);
                    code_.call(rax);     // call sin(xmm0), sin(A)
#ifdef _WIN64
                    code_.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code_.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code_.sub(rsp, 8);   // stack alignment
#endif
                    code_.mov(rax, sin_asm);
                    code_.call(rax);  // call sin(xmm0), sin(A)
#ifdef _WIN64
                    code_.add(rsp, 40);
#else
                    code_.add(rsp, 8);
#endif
                }

                code_.pop(P); // restore the P register
                break;
            }
            case COS_R: {
                // A = cos(registers_[P % registerLength]);
                code_.movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code_.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code_.sub(rsp, 32);  // windows shadow space
#endif
                    code_.mov(rax, cos_asm);
                    code_.call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    code_.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code_.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code_.sub(rsp, 8);   // stack alignment
#endif
                    code_.mov(rax, cos_asm);
                    code_.call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    code_.add(rsp, 40);
#else
                    code_.add(rsp, 8);
#endif
                }

                code_.pop(P); // restore the P register
                break;
            }
            case EXP_R: {
                // A = exp(registers_[P % registerLength]);
                code_.movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code_.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code_.sub(rsp, 32);  // windows shadow space
#endif
                    code_.mov(rax, exp_asm);
                    code_.call(rax);     // call exp(xmm0), exp(A
#ifdef _WIN64
                    code_.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code_.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code_.sub(rsp, 8);   // stack alignment
#endif
                    code_.mov(rax, exp_asm);
                    code_.call(rax);     // call exp(xmm0), exp(A
#ifdef _WIN64
                    code_.add(rsp, 40);
#else
                    code_.add(rsp, 8);
#endif
                }

                code_.pop(P); // restore the P register
                break;
            }
            case ADD_I: {
                // A += _inputs[P % inputLength];
                code_.addsd(A, ptr[inputs + PI * 8]); // add to A
                break;
            }
            case SUB_I: {
                // A -= _inputs[P % inputLength];
                code_.subsd(A, ptr[inputs + PI * 8]); // sub from A
                break;
            }
            case DIV_I: {
                // A /= _inputs[P % inputLength];
                code_.divsd(A, ptr[inputs + PI * 8]); // div A
                break;
            }
            case MUL_I: {
                // A *= _inputs[P % inputLength];
                code_.mulsd(A, ptr[inputs + PI * 8]); // mul with A
                break;
            }
            case SIN_I: {
                // A = sin(_inputs[P % inputLength]);
                code_.movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code_.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code_.sub(rsp, 32);  // windows shadow space
#endif
                    code_.mov(rax, sin_asm);
                    code_.call(rax);     // call sin(xmm0), sin(A)
#ifdef _WIN64
                    code_.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code_.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code_.sub(rsp, 8);   // stack alignment
#endif
                    code_.mov(rax, sin_asm);
                    code_.call(rax);  // call sin(xmm0), sin(A)
#ifdef _WIN64
                    code_.add(rsp, 40);
#else
                    code_.add(rsp, 8);
#endif
                }

                code_.pop(P); // restore the P register
                break;
            }
            case COS_I: {
                // A = cos(_inputs[P % inputLength]);
                code_.movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code_.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code_.sub(rsp, 32);  // windows shadow space
#endif
                    code_.mov(rax, cos_asm);
                    code_.call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    code_.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code_.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code_.sub(rsp, 8);   // stack alignment
#endif
                    code_.mov(rax, cos_asm);
                    code_.call(rax);     // call cos(xmm0), cos(A)
#ifdef _WIN64
                    code_.add(rsp, 40);
#else
                    code_.add(rsp, 8);
#endif
                }

                code_.pop(P); // restore the P register
                break;
            }
            case EXP_I: {
                // A = exp(_inputs[P % inputLength]);
                code_.movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code_.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code_.sub(rsp, 32);  // windows shadow space
#endif
                    code_.mov(rax, exp_asm);
                    code_.call(rax);     // call exp(xmm0), exp(A)
#ifdef _WIN64
                    code_.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code_.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code_.sub(rsp, 8);   // stack alignment
#endif
                    code_.mov(rax, exp_asm);
                    code_.call(rax);     // call exp(xmm0), exp(A)
#ifdef _WIN64
                    code_.add(rsp, 40);
#else
                    code_.add(rsp, 8);
#endif
                }

                code_.pop(P); // restore the P register
                break;
            }
            case INC: {
                // P++;
                code_.add(P, 1); // add 1
                // prepare rax, because cmove does not support immediate addressing
                code_.xor_(rax, rax); // rax = 0
                // update PI
                code_.add(PI, 1); // add 1
                code_.cmp(PI, inputLength); // compare to length
                code_.cmove(PI, rax); // set to 0 if equal length
                // update PR
                code_.add(PR, 1); // add 1
                code_.cmp(PR, registerLength); // compare to length
                code_.cmove(PR, rax); // set to 0 if equal length
                break;
            }
            case DEC: {
                // P--;
                code_.sub(P, 1); // sub 1
                // update PI
                code_.cmp(PI, 0); // compare to 0
                code_.cmove(PI, inputLength); // set to length if equal 0
                code_.sub(PI, 1); // sub 1
                // update PR
                code_.cmp(PR, 0); // compare to 0
                code_.cmove(PR, registerLength); // set to length if equal 0
                code_.sub(PR, 1); // sub 1

                break;
            }
            case RES: {
                // P = 0;
                code_.xor_(P, P); // set to 0
                code_.xor_(PI, PI); // set to 0
                code_.xor_(PR, PR); // set to 0
                break;
            }
            case SET: {
                // A = _constants[_counter++ % _constantsLength];
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code_.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code_.sub(rsp, 32);  // windows shadow space
#endif
                    code_.mov(rax, constants);
                    code_.call(rax);     // call constants
#ifdef _WIN64
                    code_.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code_.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code_.sub(rsp, 8);   // stack alignment
#endif
                    code_.mov(rax, constants);
                    code_.call(rax);     // call constants
#ifdef _WIN64
                    code_.add(rsp, 40);
#else
                    code_.add(rsp, 8);
#endif
                }

                code_.pop(P); // restore the P register
                break;
            }
            case FOR: {
                instructionStack.push_back(FOR);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);     // create end label
                Xbyak::Label start;
                startLabelStack.push_back(start); // create start label
                // prepare the for loop
                code_.push(P);           // save P to stack
                stackSize++;             // increment stack size
                code_.add(dynamicStackSize, 1);
                code_.xor_(P, P);        // P = 0
                // we assume the inputLength is >= 1
                // that means the for loop will execute al least once
                // we don't have to check the condition the first time
                // loop start
                code_.L(startLabelStack.back());
                // move process time forward
//                code_.mov(rax, processTime);        // make a copy in rax
//                code_.add(rax, processTimeCounter); // add the amount of instructions
//                code_.mov(processTime, rax);        // save to process time
//                code_.cmp(rax, maxProcessTime);
//                code_.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
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
                code_.jmp(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to the end
                // loop start
                code_.L(startLabelStack.back());
                // move process time forward
//                code_.mov(rax, processTime);        // make a copy in rax
//                code_.add(rax, processTimeCounter); // add the amount of instructions
//                code_.mov(processTime, rax);        // save to process time
//                code_.cmp(rax, maxProcessTime);
//                code_.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
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
                code_.jmp(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to the end
                // loop start
                code_.L(startLabelStack.back());
                // move process time forward
//                code_.mov(rax, processTime);        // make a copy in rax
//                code_.add(rax, processTimeCounter); // add the amount of instructions
//                code_.mov(processTime, rax);        // save to process time
//                code_.cmp(rax, maxProcessTime);
//                code_.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
//                processTimeCounter = 0;
                break;
            }
            case JMP_I: {
                instructionStack.push_back(JMP_I);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                code_.movsd(xmm1, ptr[inputs + PI * 8]); // xmm1 = I[P % length]
                code_.comisd(A, xmm1);           // compare A and xmm1
                code_.jnb(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if A >= xmm1
                break;
            }
            case JMP_R: {
                instructionStack.push_back(JMP_R);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                code_.movsd(xmm1, ptr[registers + PR * 8]); // xmm1 = R[P % length]
                code_.comisd(A, xmm1);           // compare A and xmm1
                code_.jnb(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if A >= xmm1
                break;
            }
            case JMP_P: {
                instructionStack.push_back(JMP_P);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                code_.cvtsi2sd(xmm1, P);         // xmm1 = (double) P
                code_.comisd(xmm1, A);           // compare xmm1 and A
                code_.jnb(endLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if xmm1 >= A
                break;
            }
            case END: {
                // assign label if there is one, else do nothing
                if (!endLabelStack.empty()) {
                    switch (instructionStack.back()) {
                        case FOR: {
                            code_.L(endLabelStack.back()); // bind the end label
                            code_.add(P, 1);               // increment P
                            code_.cmp(P, inputLength);     // compare with length
                            code_.jnge(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if P < length
                            // end loop
                            code_.pop(P);            // restore P
                            stackSize--;             // decrement stack size
                            code_.sub(dynamicStackSize, 1);
                            // pop all the stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                            startLabelStack.pop_back();
                            break;
                        }
                        case LOP_A: {
                            code_.L(endLabelStack.back());     // bind the end label
                            code_.movsd(xmm1, ptr[inputs + PI * 8]); // xmm1 = I[P % length]
                            code_.comisd(A, xmm1);             // compare A and xmm1
                            code_.jg(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if A < xmm1
                            // end loop, pop all the stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                            startLabelStack.pop_back();
                            break;
                        }
                        case LOP_P: {
                            code_.L(endLabelStack.back());     // bind the end label
                            code_.mov(rax, P);                 // save P to rax
                            code_.cmp(rax, inputLength);       // compare P and input length
                            code_.jg(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if P < inputLength
                            // end loop, pop all the stacks
                            instructionStack.pop_back();
                            endLabelStack.pop_back();
                            startLabelStack.pop_back();
                            break;
                        }
                        default: {
                            // this case is for JMP_I, JMP_R, JMP_P
                            code_.L(endLabelStack.back()); // bind the end label
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
                code_.push(P);           // save the P register, we'll use it
                if (stackSize % 2 == 0) {
#ifdef _WIN64
                    code_.sub(rsp, 32);  // windows shadow space
#endif
                    code_.mov(rax, rng);
                    code_.call(rax);     // call rng
#ifdef _WIN64
                    code_.add(rsp, 32);
#endif
                } else {
#ifdef _WIN64
                    code_.sub(rsp, 40);  // windows shadow space and stack alignment
#else
                    code_.sub(rsp, 8);   // stack alignment
#endif
                    code_.mov(rax, rng);
                    code_.call(rax);     // call rng
#ifdef _WIN64
                    code_.add(rsp, 40);
#else
                    code_.add(rsp, 8);
#endif
                }

                code_.pop(P); // restore the P register
            }
            default: {
                break;
            }
        }
        // increase process time and check if it's the end
        code_.mov(rax, processTime); // make a copy in rax
        code_.add(rax, 1);           // add 1
        code_.mov(processTime, rax); // save to process time
        code_.cmp(rax, maxProcessTime);
        code_.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
        // TODO make it faster, do it every 10 times and on loop entry TESTING
//        processTimeCounter++;
//        if (processTimeCounter == spaceBetweenProcessTime) {
//            code_.mov(rax, processTime);             // make a copy in rax
//            code_.add(rax, spaceBetweenProcessTime); // add step amount
//            code_.mov(processTime, rax);             // save to process time
//            code_.cmp(rax, maxProcessTime);
//            code_.ja(endProgram, Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump to end
//            processTimeCounter = 0;
//        }
    }
    // put unused labels at the end
    while (!endLabelStack.empty()) {
        switch (instructionStack.back()) {
            case FOR: {
                code_.L(endLabelStack.back()); // bind the end label
                code_.add(P, 1);               // increment P
                code_.cmp(P, inputLength);     // compare with length
                code_.jnge(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if P < length
                // end loop
                code_.pop(P);            // restore P
                stackSize--;             // decrement stack size
                code_.sub(dynamicStackSize, 1);
                // pop all the stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
                startLabelStack.pop_back();
                break;
            }
            case LOP_A: {
                code_.L(endLabelStack.back());     // bind the end label
                code_.movsd(xmm1, ptr[inputs + PI * 8]); // xmm1 = I[P % length]
                code_.comisd(A, xmm1);             // compare A and xmm1
                code_.jnle(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if A <= xmm1
                // end loop, pop all the stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
                startLabelStack.pop_back();
                break;
            }
            case LOP_P: {
                code_.L(endLabelStack.back());     // bind the end label
                code_.mov(rax, P);                 // save P to rax
                code_.cmp(rax, inputLength);       // compare P and input length
                code_.jng(startLabelStack.back(), Xbyak::CodeGenerator::LabelType::T_NEAR); // long jump if P <= inputLength
                // end loop, pop all the stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
                startLabelStack.pop_back();
                break;
            }
            default: {
                // this case is for JMP_I, JMP_R, JMP_P
                code_.L(endLabelStack.back()); // bind the end label
                // pop end and instruction stacks
                instructionStack.pop_back();
                endLabelStack.pop_back();
            }
        }
    }
    // end of the program
    code_.L(endProgram);
    code_.mov(rax, dynamicStackSize); // prepare for stack restoration
    code_.shl(rax, 3);                // * 8, move stack by 8 for every push
    code_.add(rsp, rax);              // pop from stack
    code_.mov(rax, processTime); // return process time
//    code_.add(rax, spaceBetweenProcessTime - processTimeCounter - 1); // add remaining process time
    code_.add(rsp, LOCALS); // restore stack of locals
    // restore the caller's stack
    code_.pop(r15);
    code_.pop(r14);
    code_.pop(r13);
    code_.pop(r12);
    code_.pop(rbx);
    code_.pop(rbp);
    code_.ret();    // return from function

    // finalize and get function pointer
    code_.ready();
    return code_.getCode<run_fn_t>();
}
