#include <iostream>
#include "include/GAsmParser.h"
#include "include/GAsm.h"
#include "GAsmInterpreter.h"
#include "include/jit_moves.h"

void exampleEvolution();
void exampleFromFile();
void exampleParsingRandom();
void exampleParsingFile();
void exampleFib();

void exampleJIT() {
    std::vector<uint8_t> prog = { MOV_A_P, MOV_P_A, MOV_A_P };

    run_fn2_t fn = compile_moves(prog);

    // Prepare runtime buffers
    double inputs[4] = {1.0,2.0,3.0,4.0};
    double regs[8]; std::memset(regs, 0, sizeof(regs));
    double constants[4] = {0.1,0.2,0.3,0.4};

    // rng function
    auto rng = []()->double { return 0.42; };

    // Call JITed function
    fn(inputs, 4, regs, 8, constants, 4, +[](){ return 0.0; }); // dummy constant and rng

    std::cout << "Done JIT\n";
}

void exampleCompile() {
    // work: MOV_P_A, MOV_A_P, DEC
    // bugs: too many instructinos: MOV_P_A, MOV_A_P, INC, DEC
    // don't work: everything with registers and inputs
    // bad mem size with RNG, SET
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

    std::vector<double> constants = {1,2,3};
    auto runner = GAsmInterpreter(program, 2, constants);
    run_fn_t compiled = runner.compile(program); // compile the program to assembly

    auto* inputs = new double[]{7.0};
    double regs[2]; std::memset(regs, 0, sizeof(regs));

    size_t time = compiled(inputs, 1, regs, 2, +[](){ return 0.0; }, +[](){ return 0.0; }); // dummy constant and rng

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
    exampleCompile();
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
    std::vector<double> constants = {1,2,3};
    auto runner = GAsmInterpreter(program, 2, constants);
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