// jit_moves.cpp
#include "xbyak.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <functional>
#include <memory>
#include <cmath>
#include "GAsmParser.h"

// function type returned by compile(...)
using run_fn2_t = void (*)(double* inputs, size_t inputLength,
                          double* registers, size_t registerLength,
                          double* constants, size_t constantLength,
                          double (*rng)());

// Compiles a program in bytecode
// Returns a function pointer you can call directly.
run_fn2_t compile_moves(const std::vector<uint8_t>& program)
{
    struct Jit : Xbyak::CodeGenerator {
        // 4096 default program size
        // (void*) 1 - auto grow buffer
        Jit() : Xbyak::CodeGenerator(4096, (void*)1) {}
    };

    auto code = std::make_unique<Jit>();

    // Register mapping (System V AMD64)
    // 1: rdi -> inputs (double*)
    // 2: rsi -> inputLength (size_t)
    // 3: rdx -> registers (double*)
    // 4: rcx -> registerLength (size_t)
    // 5: r8  -> constants (double*)
    // 6: r9  -> constantLength (size_t)
    // 7th arg (rng) is on stack at [rsp + 8] at function entry (before pushing rbp)
    //
    // We'll use these callee-saved regs:
    // r12 -> P (size_t)
    // r13 -> saved registers pointer (double*)
    // r14 -> saved inputs pointer (double*)
    // r15 -> saved constants pointer (double*)
    //
    // A (accumulator) we will keep in XMM0
    //
    // We'll preserve r12-r15 per ABI.

    // --- prologue ---
    code->push(Xbyak::util::rbp);
    code->mov(Xbyak::util::rbp, Xbyak::util::rsp);

    // save callee-saved we will use
    code->push(Xbyak::util::r12);
    code->push(Xbyak::util::r13);
    code->push(Xbyak::util::r14);
    code->push(Xbyak::util::r15);

    // zero P (r12) and A (xmm0)
    code->mov(Xbyak::util::r12, 0);               // P = 0
    code->pxor(Xbyak::util::xmm0, Xbyak::util::xmm0);   // A = 0.0

    // move pointers into callee-saved for easy use
    code->mov(Xbyak::util::r13, Xbyak::util::rdx); // registers -> r13
    code->mov(Xbyak::util::r14, Xbyak::util::rdi); // inputs -> r14
    code->mov(Xbyak::util::r15, Xbyak::util::r8);  // constants -> r15

    // load rng pointer (7th arg) from stack into r11
    // On entry: rbp points to old rsp; rbp+8 = return addr, rbp+16 = 7th arg (rng)
    code->mov(Xbyak::util::r11, Xbyak::util::ptr[Xbyak::util::rbp + 16]); // r11 = rng

    // --- emit code for each opcode in program, unrolled ---
    for (size_t i = 0; i < program.size(); ++i) {
        uint8_t opcode = program[i];

        switch (opcode) {
            case MOV_P_A: {
                // P = (size_t) round(A)  -> use CVTTSD2SI to convert (trunc)
                // cvttsd2si rax, xmm0
                // mov r12, rax
                code->cvttsd2si(Xbyak::util::rax, Xbyak::util::xmm0); // convert double to signed 64-bit in rax
                code->mov(Xbyak::util::r12, Xbyak::util::rax);        // P = rax
                break;
            }

            case MOV_A_P: {
                // A = (double) P
                // cvtsi2sd xmm0, r12
                code->cvtsi2sd(Xbyak::util::xmm0, Xbyak::util::r12); // convert integer -> double in xmm0
                break;
            }

            case MOV_A_R: {
                // A = registers[P % registerLength]
                // We'll perform unsigned division: quotient = r12 / rcx ; remainder in rdx
                // Save rax, rdx because we'll use them (rax, rdx are caller-saved but we can use them)
                // Algorithm:
                // mov rax, r12
                // xor rdx, rdx
                // test rcx, rcx ; if rcx==0 do nothing (avoid div by zero)
                // je .skip
                // div rcx
                // lea rax, [r13 + rdx*8]
                // movsd xmm0, qword ptr [rax]
                // .skip:
                code->test(Xbyak::util::rcx, Xbyak::util::rcx);
                Xbyak::Label skipA_R = code->L();
                code->jz(skipA_R);

                code->mov(Xbyak::util::rax, Xbyak::util::r12);
                code->xor_(Xbyak::util::rdx, Xbyak::util::rdx);
                code->div(Xbyak::util::rcx); // unsigned divide rdx:rax / rcx -> rax=quot, rdx=rem

                // address = r13 + rdx * 8
                code->lea(Xbyak::util::rax, Xbyak::util::ptr[Xbyak::util::r13 + Xbyak::util::rdx * 8]);
                code->movsd(Xbyak::util::xmm0, Xbyak::util::ptr[Xbyak::util::rax]); // xmm0 = memory at address

                code->L(skipA_R);
                break;
            }

            case MOV_A_I: {
                // A = inputs[P % inputLength]
                // divisor is rsi
                code->test(Xbyak::util::rsi, Xbyak::util::rsi);
                Xbyak::Label skipA_I = code->L();
                code->jz(skipA_I);

                code->mov(Xbyak::util::rax, Xbyak::util::r12);
                code->xor_(Xbyak::util::rdx, Xbyak::util::rdx);
                code->div(Xbyak::util::rsi); // unsigned divide by inputLength
                code->lea(Xbyak::util::rax, Xbyak::util::ptr[Xbyak::util::r14 + Xbyak::util::rdx * 8]);
                code->movsd(Xbyak::util::xmm0, Xbyak::util::ptr[Xbyak::util::rax]);

                code->L(skipA_I);
                break;
            }

            case MOV_R_A: {
                // registers[P % registerLength] = A
                // similar division and store
                code->test(Xbyak::util::rcx, Xbyak::util::rcx);
                Xbyak::Label skipR_A = code->L();
                code->jz(skipR_A);

                code->mov(Xbyak::util::rax, Xbyak::util::r12);
                code->xor_(Xbyak::util::rdx, Xbyak::util::rdx);
                code->div(Xbyak::util::rcx);

                code->lea(Xbyak::util::rax, Xbyak::util::ptr[Xbyak::util::r13 + Xbyak::util::rdx * 8]);
                code->movsd(Xbyak::util::ptr[Xbyak::util::rax], Xbyak::util::xmm0);

                code->L(skipR_A);
                break;
            }

            case MOV_I_A: {
                // inputs[P % inputLength] = A
                code->test(Xbyak::util::rsi, Xbyak::util::rsi);
                Xbyak::Label skipI_A = code->L();
                code->jz(skipI_A);

                code->mov(Xbyak::util::rax, Xbyak::util::r12);
                code->xor_(Xbyak::util::rdx, Xbyak::util::rdx);
                code->div(Xbyak::util::rsi);

                code->lea(Xbyak::util::rax, Xbyak::util::ptr[Xbyak::util::r14 + Xbyak::util::rdx * 8]);
                code->movsd(Xbyak::util::ptr[Xbyak::util::rax], Xbyak::util::xmm0);

                code->L(skipI_A);
                break;
            }

            default:
                // Unhandled opcode: no-op (you can emit a trap or comment)
                break;
        } // switch opcode
    } // for program

    // epilogue: restore callee-saved and return
    code->pop(Xbyak::util::r15);
    code->pop(Xbyak::util::r14);
    code->pop(Xbyak::util::r13);
    code->pop(Xbyak::util::r12);
    code->pop(Xbyak::util::rbp);
    code->ret();

    // finalize and get function pointer
    auto ptr = code->getCode<void (*)()>();
    // transfer ownership - keep code alive (must not free while function used)
    // We'll leak code pointer here for simplicity; in production store unique_ptr<Jit> somewhere.
    // For demo we return the pointer and deliberately release ownership, but keep `code` alive by releasing pointer.
    // To keep it simple: release unique_ptr so code memory stays allocated until program exit.
    // Note: xbyak uses mmap internally and will free on destructor; to keep code executable, we release ownership.
    code.release();
    return reinterpret_cast<run_fn2_t>(ptr);
}
