#include <iostream>
#include "include/GAsmParser.h"
#include "include/GAsm.h"

void exampleEvolution();
void exampleFromFile();
void exampleParsingRandom();
void exampleParsingFile();

int main() {
//    exampleParsingRandom();
//    exampleParsingFile();
    exampleEvolution();
//    exampleFromFile();
    return 0;
}

void exampleEvolution() {
    auto gasm = GAsm();
    gasm.maxGenerations = 0;
    gasm.populationSize = 100;
    gasm.individualMaxSize = 20;
    auto inputs = std::vector<std::vector<double>>();
    auto targets = std::vector<std::vector<double>>();
    gasm.crossoverFunction = GAsm::onePointCrossover;
    gasm.growFunction = GAsm::treeGrow;
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
    auto gasm = GAsmParser("../data/asm.txt");
    std::cout << gasm << std::endl;
}
