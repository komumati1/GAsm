#include <vector>
#include <iostream>
#include <ntdef.h>
#include <profileapi.h>
#include "GAsmParser.h"
#include "GAsmInterpreter.h"

size_t fib(double* inputs, size_t inputLength, double* registers, size_t registerLength, double (*constants)(), double (*rng)()) {
    int processTime = 0;
    double p2 = 1;
//    processTime++; if (processTime > 10000) goto end;
    double p1 = 0;
    double p0;
//    processTime++; if (processTime > 10000) goto end;
    for (int i = 0; i < inputs[0]; i++) {
//        processTime++; if (processTime > 10000) goto end;
        p0 = p1 + p2;
//        processTime++; if (processTime > 10000) goto end;
        p2 = p1;
//        processTime++; if (processTime > 10000) goto end;
        p1 = p0;
//        processTime++; if (processTime > 10000) goto end;
    }
//end:
//    processTime++;
    inputs[0] = p0;
    return processTime;
}

void test() {
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
    std::vector<double> constants = {2.0};
    auto runner = GAsmInterpreter(program, 2);
    run_fn_t compiled = runner.compile(); // compile the program to assembly

    double* inputs;
    double regs[3]; memset(regs, 0, sizeof(regs));

    for (int j = 0; j < 1000; j++) {
        inputs = new double[]{1000.0};
//        time = compiled(inputs, 1, regs, 3, +[]() { return 0.0; }, +[]() { return 0.0; }); // dummy constant and rng
        fib(inputs, 1, regs, 3, +[](){ return 0.0; }, +[](){ return 0.0; });
    }

    LARGE_INTEGER start, finish, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
// Do something
    size_t time;
    for (int j = 0; j < 1000; j++) {
        std::vector<double> yes = {1000.0};
//        time = compiled(inputs, 1, regs, 3, +[]() { return 0.0; }, +[]() { return 0.0; }); // dummy constant and rng
//        time = fib(inputs, 1, regs, 3, +[](){ return 0.0; }, +[](){ return 0.0; });
        size_t processTime = runner.run(yes, 10000);
    }

    QueryPerformanceCounter(&finish);
    std::cout << "Execution took "
              << ((double)(finish.QuadPart - start.QuadPart) / (double)freq.QuadPart) << std::endl;
    std::cout << "Process time: " << time << std::endl;
    std::cout << "Result: " << inputs[0] << std::endl;

    for (int j = 0; j < 1000; j++) {
        inputs = new double[]{1000.0};
        compiled(inputs, 1, regs, 3, +[]() { return 0.0; }, +[]() { return 0.0; }, 10000); // dummy constant and rng
//        fib(inputs, 1, regs, 3, +[](){ return 0.0; }, +[](){ return 0.0; });
    }

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
// Do something
    for (int j = 0; j < 1000; j++) {
        inputs = new double[]{1000.0};
        run_fn_t compiled = runner.compile(); // compile the program to assembly
        time = compiled(inputs, 1, regs, 3, +[]() { return 0.0; }, +[]() { return 0.0; }, 10000); // dummy constant and rng
//        time = fib(inputs, 1, regs, 3, +[](){ return 0.0; }, +[](){ return 0.0; });
    }

    QueryPerformanceCounter(&finish);
    std::cout << "Execution took "
              << ((double)(finish.QuadPart - start.QuadPart) / (double)freq.QuadPart) << std::endl;
    std::cout << "Process time: " << time << std::endl;
    std::cout << "Result: " << inputs[0] << std::endl;
}