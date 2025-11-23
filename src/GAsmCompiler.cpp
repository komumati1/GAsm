
#include <vector>
#include <cmath>
#include <xbyak.h>
#include <memory>
#include "GAsmInterpreter.h"
#include "GAsmParser.h"

run_fn_t GAsmInterpreter::compile(const std::vector<uint8_t>& program) {
    using namespace Xbyak::util;

    struct Jit : Xbyak::CodeGenerator {
        // 4096 default program size
        // (void*) 1 - auto grow buffer
        Jit() : Xbyak::CodeGenerator(1, Xbyak::AutoGrow) {}
    };

    auto code = std::make_unique<Jit>();
    // used for nested loops and ifs
    auto endLabelStack = std::vector<Xbyak::Label>();
    auto startLabelStack = std::vector<Xbyak::Label>();
    auto instructionStack = std::vector<uint8_t>();

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
    // QWORD PTR [rbp - 8] -> constants function<double()>
    #define constants qword[rbp - 8]
    // QWORD PTR [rbp - 16] -> rng function<double()>
    #define rng qword[rbp - 16]
    // rrbx -> P (size_t)
    #define P rbx
    // r13 -> saved inputs pointer (double*)
    #define inputs r14
    // QWORD PTR [rbp - 24] -> saved input length (size_t)
    #define inputLength qword[rbp - 24]
    // r14 -> saved registers pointer (double*)
    #define registers r15
    // QWORD PTR [rbp - 32] -> saved registers length (size_t)
    #define registerLength qword[rbp - 32]
    // QWORD PTR [rbp - 40] -> process time (size_t)
    #define processTime qword[rbp - 40]
    // QWORD PTR [rbp - 48] -> stack size for saved P used in for loop (size_t)
    #define stackSize qword[rbp - 48]
    // xmm0 -> A (accumulator) (double)
    #define A xmm0
    // WARNING!!! CHANGE LOCAL STACK SIZE IF NEEDED
    #define LOCALS 48
    //
    // Registers free to use in the program
    // xmm1-15 - for floating numbers
    // rax, rdx, rcx

    // --- SETUP ---
    // push new frame pointer
    code->push(rbp);
    // create new stack pointer
    code->mov(rbp, rsp);
    // FIXME TESTS
//    code->mov(rax, r9); //rdi = 0, rsi, rdx = 1, rcx = inputs, r8 = registers, r9 = 2 (size of registers)
//    code->pop(rbp);
//    code->ret();

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
    // LINUX FIXME
#ifdef __unix__
    code->mov(inputs, rdi);
    code->mov(inputLength, rsi);
    code->mov(registers, rdx);
    code->mov(registerLength, rcx);
    code->mov(constants, r8);  // constants
    code->mov(rng, r9);        // rng;
#elifdef VOS__WINDOWS16
    code->mov(inputs, rcx);
    code->mov(inputLength, rdx);
    code->mov(registers, r8);
    code->mov(registerLength, r9);
    code->mov(rax, qword[rsp + 40]);
    code->mov(constants, rax);  // constants
    code->mov(rax, qword[rsp + 48]);
    code->mov(rng, rax);        // rng;
#endif

    // reset P, PI, PR, A and processTime
    code->xor_(P, P);            // P = 0
    code->pxor(A, A);            // A = 0.0
    code->xor_(PI, PI);          // PI = 0
    code->xor_(PR, PR);          // PR = 0
    code->mov(processTime, 0); // processTime = 0;
    code->mov(stackSize, 0);   // stackSize = 0;

    // prepare end program label
    Xbyak::Label endProgram;
    // TODO labels are incorrect
    // --- COMPILATION ---
    for (int i = 0; i < program.size(); i++) {
        const uint8_t& opcode = program[i];

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
                code->mov(P, stackSize); // mov stackSize to P
                code->and_(P, 1);        // check the parity (only the last bit)
                code->shl(P, 3);         // multiply by 8
                // P has not 8 if stackSize is odd and 0 if it's even
                code->sub(rsp, P);       // align the stack 16 bytes
                code->call((void*)sin);  // call sin(xmm0), sin(A)
                code->add(rsp, P);       // restore the stack, xmm0 = A = sin(A)

                code->pop(P); // restore the P register
                break;
            }
            case COS_R: {
                // A = cos(_registers[P % registerLength]);
                code->movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                code->mov(P, stackSize); // mov stackSize to P
                code->and_(P, 1);        // check the parity (only the last bit)
                code->shl(P, 3);         // multiply by 8
                // P has not 8 if stackSize is odd and 0 if it's even
                code->sub(rsp, P);       // align the stack 16 bytes
                code->call((void*)cos);  // call cos(xmm0), cos(A)
                code->add(rsp, P);       // restore the stack, xmm0 = A = cos(A)

                code->pop(P); // restore the P register
                break;
            }
            case EXP_R: {
                // A = exp(_registers[P % registerLength]);
                code->movsd(A, ptr[registers + PR * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                code->mov(P, stackSize); // mov stackSize to P
                code->and_(P, 1);        // check the parity (only the last bit)
                code->shl(P, 3);         // multiply by 8
                // P has not 8 if stackSize is odd and 0 if it's even
                code->sub(rsp, P);       // align the stack 16 bytes
                code->call((void*)exp);  // call exp(xmm0), exp(A)
                code->add(rsp, P);       // restore the stack, xmm0 = A = exp(A)

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
                code->mov(P, stackSize); // mov stackSize to P
                code->and_(P, 1);        // check the parity (only the last bit)
                code->shl(P, 3);         // multiply by 8
                // P has not 8 if stackSize is odd and 0 if it's even
                code->sub(rsp, P);       // align the stack 16 bytes
                code->call((void*)sin);  // call sin(xmm0), sin(A)
                code->add(rsp, P);       // restore the stack, xmm0 = A = sin(A)

                code->pop(P); // restore the P register
                break;
            }
            case COS_I: {
                // A = cos(_inputs[P % inputLength]);
                code->movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                code->mov(P, stackSize); // mov stackSize to P
                code->and_(P, 1);        // check the parity (only the last bit)
                code->shl(P, 3);         // multiply by 8
                // P has not 8 if stackSize is odd and 0 if it's even
                code->sub(rsp, P);       // align the stack 16 bytes
                code->call((void*)cos);  // call cos(xmm0), cos(A)
                code->add(rsp, P);       // restore the stack, xmm0 = A = cos(A)

                code->pop(P); // restore the P register
                break;
            }
            case EXP_I: {
                // A = exp(_inputs[P % inputLength]);
                code->movsd(A, ptr[inputs + PI * 8]); // assign from pointer to A
                // WARNING!!! ONLY ODD NUMBER OF VARIABLES CAN BE ON THE STACK
                code->push(P);           // save the P register, we'll use it
                code->mov(P, stackSize); // mov stackSize to P
                code->and_(P, 1);        // check the parity (only the last bit)
                code->shl(P, 3);         // multiply by 8
                // P has not 8 if stackSize is odd and 0 if it's even
                code->sub(rsp, P);       // align the stack 16 bytes
                code->call((void*)exp);  // call exp(xmm0), exp(A)
                code->add(rsp, P);       // restore the stack, xmm0 = A = exp(A)

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
                code->mov(P, stackSize); // mov stackSize to P
                code->and_(P, 1);        // check the parity (only the last bit)
                code->shl(P, 3);         // multiply by 8
                // P has not 8 if stackSize is odd and 0 if it's even
                code->sub(rsp, P);       // align the stack 16 bytes
                code->call(constants);   // call constants
                code->add(rsp, P);       // restore the stack, xmm0 = A = constants()

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
                code->add(stackSize, 1); // increment stack size
                code->xor_(P, P);        // P = 0
                // we assume the inputLength is >= 1
                // that means the for loop will execute al least once
                // we don't have to check the condition the first time
                // loop start
                code->L(startLabelStack.back()); // FIXME maybe label to nop?
                break;
            }
            case LOP_A: {
                instructionStack.push_back(LOP_A); // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);      // create end label
                Xbyak::Label start;
                startLabelStack.push_back(start);  // create start label
                // condition is at the end
                code->jmp(endLabelStack.back(), Jit::T_NEAR); // jump to the end TODO fix long jump
                // loop start
                code->L(startLabelStack.back()); // FIXME maybe label to nop?
                break;
            }
            case LOP_P: {
                instructionStack.push_back(LOP_P); // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);      // create end label
                Xbyak::Label start;
                startLabelStack.push_back(start);  // create start label
                // condition is at the end
                code->jmp(endLabelStack.back()); // jump to the end
                // loop start
                code->L(startLabelStack.back()); // FIXME maybe label to nop?
                break;
            }
            case JMP_I: {
                instructionStack.push_back(JMP_I);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                code->movsd(xmm1, ptr[inputs + PI*8]); // xmm1 = I[P % length]
                code->comisd(A, xmm1);           // compare A and xmm1
                code->jnb(endLabelStack.back()); // jump if A >= xmm1
                break;
            }
            case JMP_R: {
                instructionStack.push_back(JMP_R);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                code->movsd(xmm1, ptr[registers + PR*8]); // xmm1 = R[P % length]
                code->comisd(A, xmm1);           // compare A and xmm1
                code->jnb(endLabelStack.back()); // jump if A >= xmm1
                break;
            }
            case JMP_P: {
                instructionStack.push_back(JMP_P);  // notify END about the instruction
                Xbyak::Label end;
                endLabelStack.push_back(end);    // create end label
                code->cvtsi2sd(xmm1, P);         // xmm1 = (double) P
                code->comisd(xmm1, A);           // compare xmm1 and A
                code->jnb(endLabelStack.back()); // jump if xmm1 >= A
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
                            code->jnge(startLabelStack.back()); // jump if P < length
                            // end loop
                            code->pop(P);            // restore P
                            code->sub(stackSize, 1); // decrement stack size
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
                            code->jnle(startLabelStack.back(), Jit::T_NEAR); // jump if A <= xmm1 // TODO long jump //TOO validate jump conditions
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
                            code->jng(startLabelStack.back()); // jump if P <= inputLength
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
                code->mov(P, stackSize); // mov stackSize to P
                code->and_(P, 1);        // check the parity (only the last bit)
                code->shl(P, 3);         // multiply by 8
                // P has not 8 if stackSize is odd and 0 if it's even
                code->sub(rsp, P);       // align the stack 16 bytes
                code->call(constants);   // call constants
                code->add(rsp, P);       // restore the stack, xmm0 = A = rng()

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
        code->cmp(rax, this->_maxProcessTime);
        code->ja(endProgram, Jit::T_NEAR); // TODO set better jumps, diff from long and short
    }
    // put unused labels at the end
    if (!endLabelStack.empty()) {
        switch (instructionStack.back()) {
            case FOR: {
                code->L(endLabelStack.back()); // bind the end label
                code->add(P, 1);               // increment P
                code->cmp(P, inputLength);     // compare with length
                code->jnge(startLabelStack.back()); // jump if P < length
                // end loop
                code->pop(P);            // restore P
                code->sub(stackSize, 1); // decrement stack size
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
                code->jng(startLabelStack.back()); // jump if A <= xmm1
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
                code->jng(startLabelStack.back()); // jump if P <= inputLength
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