#include <iostream>
#include "include/GAsmParser.h"
#include "include/GAsm.h"

int main() {
//    auto gasm = GAsmParser("../data/asm.txt");
//    size_t length = 128;
//    size_t zipped_length = 10;
//    auto* zipped = new uint64_t[zipped_length];
//    uint8_t *bytecode = GAsmParser::unzip(zipped, length, zipped_length);
//    std::cout << GAsmParser::bytecode2Text(bytecode, length) << std::endl;
    auto gasm = GAsm();
    gasm.maxGenerations = 10;
    gasm.populationSize = 100;
    gasm.individualMaxSize = 10;
    auto inputs = std::vector<std::vector<double>>();
    auto targets = std::vector<std::vector<double>>();
    gasm.evolve(inputs, targets);
    return 0;
}
