#include <vector>
#include <iostream>
#include <ntdef.h>
#include <profileapi.h>
#include "GAsmParser.h"
#include "GAsmInterpreter.h"

size_t fib(double* inputs, size_t inputLength, double* registers, size_t registerLength, double (*constants)(), double (*rng)(), size_t maxProcessTime) {
    int processTime = 0;
    double p2 = 1;
    double p1 = 0;
    double p0;
    for (int i = 0; i < inputs[0]; i++) {
        p0 = p1 + p2;
        p2 = p1;
        p1 = p0;
    }
    inputs[0] = p0;
    return processTime;
}

void CppvsGAsm() {
    std::vector<uint8_t> program = {
            MOV_R_A,
            INC,
            MOV_A_P,
            MOV_R_A,
            LOP_A,
            DEC,
            MOV_A_R,
            INC,
            ADD_R,
            INC,
            MOV_R_A,
            MOV_A_P,
            END,
            MOV_A_R,
            MOV_I_A
    };
    // compilation
    auto runner = GAsmInterpreter(program, 2);
    run_fn_t compiled = runner.compile(); // compile the program to assembly

    // parameters
    double fibCount = 1000.0;
    double* inputs = new double[]{fibCount};
    size_t registerLength = 2;
    double regs[registerLength]; memset(regs, 0, sizeof(regs));
    size_t maxProcessTime = 100000;
    double (*cng)() = +[](){ return 0.0; };
    double (*rng)() = +[](){ return 0.0; };

    // variables
    LARGE_INTEGER start, finish, freq;
    size_t time;

    // warm up
    for (int j = 0; j < 1000; j++) {
        inputs[0] = fibCount;
        fib(inputs, 1, regs, registerLength, cng, rng, maxProcessTime);
    }

    // measure performance
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    for (int j = 0; j < 100000; j++) {
        inputs[0] = fibCount;
        fib(inputs, 1, regs, registerLength, cng, rng, maxProcessTime);
    }

    // print performance
    QueryPerformanceCounter(&finish);
    std::cout << "Cpp version" << std::endl;
    std::cout << "Execution took "
              << ((double)(finish.QuadPart - start.QuadPart) / (double)freq.QuadPart) << std::endl;
    std::cout << "Result: " << inputs[0] << std::endl;

    // warm up
    for (int j = 0; j < 1000; j++) {
        inputs[0] = fibCount;
        compiled(inputs, 1, regs, registerLength, cng, rng, maxProcessTime);
    }

    // measure performance
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    for (int j = 0; j < 100000; j++) {
        inputs[0] = fibCount;
        compiled(inputs, 1, regs, registerLength, cng, rng, maxProcessTime);
    }

    // print performance
    QueryPerformanceCounter(&finish);
    std::cout << "GAsm version" << std::endl;
    std::cout << "Execution took "
              << ((double)(finish.QuadPart - start.QuadPart) / (double)freq.QuadPart) << std::endl;
    std::cout << "Result: " << inputs[0] << std::endl;
}