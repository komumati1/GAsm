#include <iostream>
#include <profileapi.h>
#include "GAsmParser.h"
#include "GAsm.h"
#include "GAsmInterpreter.h"
#include "jit_moves.h"
#include "fib_learn.h"
#include "tests.h"

void exampleEvolution();
void exampleFromFile();
void exampleParsingRandom();
void exampleParsingFile();
void exampleFib();

void exampleCompile() {
    std::vector<uint8_t> program = {
            LOP_P,
            RNG,
            LOP_A,
            SIN_R,
            FOR,
            INC,
            COS_I,
            MOV_A_I,
            COS_I,
            EXP_I,
            COS_R,
            MOV_A_R,
            SUB_R,
            MOV_A_I,
            SIN_R,
            FOR,
            SET,
            COS_I,
            DIV_I,
            SET
    };

    auto runner = GAsmInterpreter(program, 2);
    run_fn_t compiled = runner.compile(); // compile the program to assembly

    auto* inputs = new double[]{7.0};
    double regs[2]; std::memset(regs, 0, sizeof(regs));
    double (*cng)() = +[](){ return 0.0; };
    double (*rng)() = +[](){ return 0.0; };

    size_t time = compiled(inputs, 1, regs, 2, cng, rng, 10000); // dummy constant and rng

    std::cout << "Process time: " << time << std::endl;
    std::cout << "Result: " << inputs[0] << std::endl;
}

int main() {
//    exampleParsingRandom();
//    exampleParsingFile();
//    exampleEvolution();
//    exampleFromFile();
//    exampleFib();
//    exampleJIT();
//    exampleCompile();
//    fibEvolution();
    CppvsGAsm();
    InterpreterVsCompiler();
    return 0;
}

void exampleEvolution() {
    auto gasm = GAsm();
    gasm.maxGenerations = 10;
    gasm.populationSize = 100;
    gasm.individualMaxSize = 20;
    auto inputs = std::vector<std::vector<double>>();
    auto targets = std::vector<std::vector<double>>();
    gasm.evolve(inputs, targets);
    gasm.save2File("../data/test.json");
    std::cout << GAsmParser::bytecode2Text(gasm.bestIndividual.data(), gasm.bestIndividual.size()) << std::endl;
}

void exampleFromFile() {
    auto gasm = GAsm("../data/test.json");
    std::cout << GAsmParser::bytecode2Text(gasm.bestIndividual.data(), gasm.bestIndividual.size()) << std::endl;
}

void exampleParsingRandom() {
    size_t length = 128;
    size_t zipped_length = 10;
    auto* zipped = new uint64_t[zipped_length];
    uint8_t *bytecode = GAsmParser::unzip(zipped, length, zipped_length);
    std::cout << GAsmParser::bytecode2Text(bytecode, length) << std::endl;
}

void exampleParsingFile() {
    auto gasm = GAsmParser("../data/asm.gasm");
    std::cout << gasm << std::endl;
}

void exampleFib() {
    auto gasm = GAsmParser("../data/fib.gasm");
    std::vector<uint8_t> program(0);
    for (int i = 0; i < gasm.length(); i++) {
        program.push_back(gasm.bytecode()[i]);
    }
    auto runner = GAsmInterpreter(program, 2);
    int number;
    std::cout << "Which fib you want?" << std::endl;
    std::cin >> number;
    std::vector<double> inputs(0);
    inputs.push_back(number);
    size_t processTime = runner.run(inputs, 10000);
    for (const auto& num : inputs) {
        std::cout << num << ", ";
    }
    std::cout << std::endl << "Process time: " << processTime << std::endl;
    std::cout << (processTime == 10000 ? "Time limit too small" : "OK") << std::endl;
}