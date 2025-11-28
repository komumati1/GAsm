//
// Created by mateu on 20.11.2025.
//

#include "functions.h"
#include "GAsm.h"
#include "GAsmParser.h"

std::pair<double, double> Fitness::operator()(GAsm *self, const std::vector<uint8_t> &individual) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double score = 0.0;
    for (unsigned char i : individual) {
        if (i == 0x00) score++;
    }
    return {score, 0.0};
}

size_t TournamentSelection::operator()(const GAsm *self) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, self->getPopulation().size() - 1);

    size_t bestIndex = dist(rng);
    for (unsigned int i = 1; i < _tournamentSize; i++) {
        size_t idx = dist(rng);
        if (selectMinimal ? self->getFitness()[idx] < self->getFitness()[bestIndex] : self->getFitness()[idx] > self->getFitness()[bestIndex]) {
            bestIndex = idx;
        }
    }
    return bestIndex;
}

void OnePointCrossover::operator()(const GAsm *self, std::vector<uint8_t> &worstIndividual,
                                   const std::vector<uint8_t> &bestIndividual1,
                                   const std::vector<uint8_t> &bestIndividual2) {
    if (bestIndividual1.empty() || bestIndividual2.empty()) return;

    size_t minSize = std::min(bestIndividual1.size(), bestIndividual2.size());

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, minSize - 1);

    int crossPoint = (int)dist(rng);

    worstIndividual.resize(bestIndividual2.size());

    std::copy_n(bestIndividual1.data(), crossPoint, worstIndividual.data());
    std::copy_n(bestIndividual2.data() + crossPoint, bestIndividual2.size() - crossPoint, worstIndividual.data() + crossPoint);
}

void TwoPointCrossover::operator()(const GAsm *self, std::vector<uint8_t> &worstIndividual,
                                   const std::vector<uint8_t> &bestIndividual1,
                                   const std::vector<uint8_t> &bestIndividual2) {
    if (bestIndividual1.empty() || bestIndividual2.empty()) return;

    size_t minSize = std::min(bestIndividual1.size(), bestIndividual2.size());

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, minSize - 1);

    int crossPoint1 = (int)dist(rng);
    int crossPoint2;
    do {
        crossPoint2 = (int)dist(rng);
    } while (crossPoint1 == crossPoint2);

    if (crossPoint1 > crossPoint2) {
        std::swap(crossPoint1, crossPoint2);
    }

    worstIndividual.resize(bestIndividual1.size());

    std::copy_n(bestIndividual1.data(), crossPoint1, worstIndividual.data());
    std::copy_n(bestIndividual2.data() + crossPoint1, crossPoint2 - crossPoint1, worstIndividual.data() + crossPoint1);
    std::copy_n(bestIndividual1.data() + crossPoint2, bestIndividual2.size() - crossPoint2, worstIndividual.data() + crossPoint2);
}

void UniformPointCrossover::operator()(const GAsm *self, std::vector<uint8_t> &worstIndividual,
                                       const std::vector<uint8_t> &bestIndividual1,
                                       const std::vector<uint8_t> &bestIndividual2) {
    if (bestIndividual1.empty() || bestIndividual2.empty()) return;

    const std::vector<uint8_t>& biggerIndividual = bestIndividual1.size() > bestIndividual2.size() ? bestIndividual1 : bestIndividual2;
    const std::vector<uint8_t>& smallerIndividual = bestIndividual1.size() > bestIndividual2.size() ? bestIndividual2 : bestIndividual1;

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, 1);

    worstIndividual.resize(biggerIndividual.size());

    for (int i = 0; i < smallerIndividual.size(); i++) {
        if (dist(rng) == 1) {
            // fuck push_back and other vector methods
            worstIndividual[i] = smallerIndividual[i];
        } else {
            worstIndividual[i] = biggerIndividual[i];
        }
    }
    std::copy_n(biggerIndividual.data() + smallerIndividual.size(), biggerIndividual.size() - smallerIndividual.size(), worstIndividual.data() + smallerIndividual.size());
}

void HardMutation::operator()(const GAsm *self, std::vector<uint8_t> &worstIndividual,
                              const std::vector<uint8_t> &bestIndividual) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> probDist(0.0, 1.0);
    std::uniform_int_distribution<uint8_t> byteDist(0, 31); // bytecode range is 0-31

    worstIndividual = bestIndividual;

    for (unsigned char& i : worstIndividual) {
        if (probDist(rng) < self->mutationProbability) {
            i = GAsmParser::base322Bytecode(byteDist(rng)); // mutate this byte
        }
    }
}

static std::array<std::uniform_int_distribution<uint8_t>, INSTRUCTION_GROUPS> makeDists() {
    std::array<std::uniform_int_distribution<uint8_t>, INSTRUCTION_GROUPS> arr;

    for (int i = 0; i < INSTRUCTION_GROUPS; i++) {
        arr[i] = std::uniform_int_distribution<uint8_t>(0, GAsmParser::instructionGroupLengths[i] - 1);
    }

    return arr;
}

void SoftMutation::operator()(const GAsm *self, std::vector<uint8_t> &worstIndividual,
                              const std::vector<uint8_t> &bestIndividual) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> probDist(0.0, 1.0);

    static const std::array<std::uniform_int_distribution<uint8_t>, INSTRUCTION_GROUPS> dists = makeDists();

    worstIndividual = bestIndividual;

    for (unsigned char& i : worstIndividual) {
        if (probDist(rng) < self->mutationProbability) {
            auto dist = dists[i >> 4];
            i = dist(rng) | (i & 0b11110000); // mutate the end of this byte
        }
    }
}

void FullGrow::operator()(const GAsm *self, std::vector<uint8_t> &individual) {
    individual.clear();

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<uint8_t> dist(0, 31);

    individual.resize(self->individualMaxSize);
    std::generate(individual.begin(),
                  individual.end(),
                  [&](){ return GAsmParser::base322Bytecode(dist(engine)); });
}

void TreeGrow::operator()(const GAsm *self, std::vector<uint8_t> &individual) {
    individual.clear();
    individual.reserve(self->individualMaxSize);
    this->grow(individual, self->individualMaxSize);
}

void TreeGrow::grow(std::vector<uint8_t> &individual, size_t maxSize) { //NOLINT, it's recursive
    bool forceStructural = individual.empty();
    bool mustBeLeaf = (this->_depth == 0);
    // 10 instructions - inna niż 10 też może być
    // tree -> 10 instructions
    // continue another 10 instructions

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 3);
    bool chooseStructural = (dist(engine) == 0);

    // TODO
    // double p = 0.45 * (double(depth) / 10.0);      // startDepth
    // std::bernoulli_distribution chooseStructure(p);
    // bool chooseStructural = chooseStructure(engine);

    if ((chooseStructural && !mustBeLeaf) || forceStructural) {
        // wybieramy FOR / LOP A / LOP P / JMP ...
        std::uniform_int_distribution<int> pickDistr(0, (int)GAsmParser::structuralOpcodesLength - 1);
        uint8_t op = ( GAsmParser::structuralOpcodes[pickDistr(engine)]);

        individual.push_back(op);

        std::uniform_int_distribution<int> childCountDist(1, 4);
        int childCount = childCountDist(engine);

        for (int i = 0; i < childCount; i++) {

            if (individual.size() + 1 >= maxSize) {
                break;
            } else {
                _depth--;
                grow(individual, maxSize);
                _depth++;
            }
        }
        // END – zamyka KAŻDY rodzaj bloku
        individual.push_back(GAsmParser::end);
    }

    // zwykla instrukcja
    std::uniform_int_distribution<int> nd(0, (int)GAsmParser::normalOpcodesLength - 1);
    individual.push_back(GAsmParser::normalOpcodes[nd(engine)]);
}
