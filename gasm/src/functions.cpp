//
// Created by mateu on 20.11.2025.
//

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "functions.h"
#include "GAsm.h"
#include "GAsmParser.h"
#include <cstdlib>
#include <iostream>

std::pair<double, double> Fitness::operator()(const GAsm* self, GAsmInterpreter& jit, const std::vector<uint8_t> &individual) {
    jit.setProgram(individual);
    double score = 0.0;
    double avgTime = 0.0;
    for (int i = 0; i < self->inputs.size(); i += 1) {
        std::vector<double> input = self->inputs[i];
        const std::vector<double>& target = self->targets[i];
        avgTime += (double)jit.run(input, self->maxProcessTime);

        double diff = input[0] - target[0];
        score += std::isfinite(diff) ? std::fabs(diff) : self->nanPenalty;
    }
    avgTime /= (double)self->inputs.size();
    return {score, avgTime};
}

std::unique_ptr<FitnessFunction> Fitness::clone() const {
    return std::make_unique<Fitness>(*this);
}

static inline bool isFinite(double x) { return std::isfinite(x); }

static inline double absErr(double pred, double truth, double nanPenalty) {
    const double diff = pred - truth;
    return isFinite(diff) ? std::fabs(diff) : nanPenalty;
}

static inline long long roundToInt(double x, long long clampAbs = 1'000'000'000LL) {
    if (!std::isfinite(x)) return 0;
    long long v = (long long)std::llround(x);
    if (v >  clampAbs) v =  clampAbs;
    if (v < -clampAbs) v = -clampAbs;
    return v;
}

std::pair<double, double> FitnessAnyPositionConstant::operator()(
    const GAsm* self, GAsmInterpreter& jit, const std::vector<uint8_t>& individual
) {
    jit.setProgram(individual);

    double score = 0.0;
    double avgTime = 0.0;

    for (int i = 0; i < (int)self->inputs.size(); ++i) {
        std::vector<double> io = self->inputs[i];
        const auto& target = self->targets[i]; // target[0] = C
        avgTime += (double)jit.run(io, self->maxProcessTime);

        const double C = target[0];

        // bierzemy najlepsze trafienie "gdziekolwiek"
        double best = self->nanPenalty;
        for (double v : io) {
            // if (!std::isfinite(v)) { best = std::min(best, self->nanPenalty); continue; }

            long long vi = roundToInt(v);
            double err = std::fabs((double)(vi - C));   // błąd liczony po int
            best = std::min(best, err);
        }

        score += best;
    }

    avgTime /= (double)self->inputs.size();
    return {score, avgTime};
}

std::unique_ptr<FitnessFunction> FitnessAnyPositionConstant::clone() const {
    return std::make_unique<FitnessAnyPositionConstant>(*this);
}


static inline long long truncToInt(double x, long long clampAbs = 1'000'000'000LL) {
    if (!std::isfinite(x)) return 0; // i tak ukarzesz nanPenalty wyżej
    long long v = (long long)x;      // C++ cast: trunc w stronę zera
    if (v >  clampAbs) v =  clampAbs;
    if (v < -clampAbs) v = -clampAbs;
    return v;
}

static inline double unchangedPenalty(const std::vector<double>& before,
                                      const std::vector<double>& after,
                                      size_t startIdx,
                                      double weight,
                                      double eps = 1e-12) {
    double p = 0.0;
    const size_t n = std::min(before.size(), after.size());
    for (size_t i = startIdx; i < n; ++i) {
        if (!isFinite(before[i]) || !isFinite(after[i])) {
            if (!(std::isnan(before[i]) && std::isnan(after[i]))) p += weight;
        } else if (std::fabs(before[i] - after[i]) > eps) {
            p += weight * std::min(1.0, std::fabs(after[i] - before[i]));
        }
    }
    return p;
}
static inline double intAbsErr(long long pred, long long truth) {
    return (double)std::llabs(pred - truth);
}

std::pair<double, double> FitnessSumSub::operator()(
    const GAsm* self, GAsmInterpreter& jit, const std::vector<uint8_t>& individual
) {
    jit.setProgram(individual);

    double score = 0.0;
    double avgTime = 0.0;

    const double extraWriteWeight = 5.0;

    for (int i = 0; i < (int)self->inputs.size(); ++i) {
        std::vector<double> io = self->inputs[i];
        std::vector<double> before = io;
        const auto& target = self->targets[i];

        avgTime += (double)jit.run(io, self->maxProcessTime);

        long long pred  = truncToInt(io[0]);
        long long truth = truncToInt(target[0]);

        score += isFinite(io[0]) ? intAbsErr(pred, truth) : self->nanPenalty;
        score += unchangedPenalty(before, io, 1, extraWriteWeight);
    }

    avgTime /= (double)self->inputs.size();
    return {score, avgTime};
}

std::unique_ptr<FitnessFunction> FitnessSumSub::clone() const {
    return std::make_unique<FitnessSumSub>(*this);
}




size_t TournamentSelection::operator()(const GAsm *self) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, self->populationSize - 1);

    size_t bestIndex = dist(rng);
    for (unsigned int i = 1; i < _tournamentSize; i++) {
        size_t idx = dist(rng);
        if (selectMinimal ? self->getFitness(idx) < self->getFitness(bestIndex) : self->getFitness(idx) > self->getFitness(bestIndex)) {
            bestIndex = idx;
        }
    }
    return bestIndex;
}

std::unique_ptr<SelectionFunction> TournamentSelection::clone() const {
    return std::make_unique<TournamentSelection>(*this);
}

size_t RouletteSelection::operator()(const GAsm* self) {
    static thread_local std::mt19937 rng(std::random_device{}());
    size_t size = self->populationSize;

    std::vector<double> weights(size);

    // Convert fitness to weights
    if (selectMinimal) {
        // Lower fitness = better, so invert
        for (size_t i = 0; i < size; i++)
            weights[i] = 1.0 / (self->getFitness(i) + 1e-12);
    } else {
        // Higher fitness = better
        for (size_t i = 0; i < size; i++)
            weights[i] = self->getFitness(i) + 1e-12;
    }

    double total = std::accumulate(weights.begin(), weights.end(), 0.0);
    std::uniform_real_distribution<double> dist(0.0, total);

    double r = dist(rng);
    double acc = 0.0;

    for (size_t i = 0; i < weights.size(); i++) {
        acc += weights[i];
        if (acc >= r)
            return i;
    }

    return weights.size() - 1; // fallback
}

std::unique_ptr<SelectionFunction> RouletteSelection::clone() const {
    return std::make_unique<RouletteSelection>(*this);
}

size_t RankSelection::operator()(const GAsm* self) {
    static thread_local std::mt19937 rng(std::random_device{}());
    size_t n = self->populationSize;

    // Lower rank = better if minimizing
    std::vector<double> weights(n);

    for (size_t i = 0; i < n; i++) {
        double rank = self->getRank(i); // 0 = best
        weights[i] = selectMinimal ? ((double)n - rank) : (rank + 1);
    }

    double total = std::accumulate(weights.begin(), weights.end(), 0.0);
    std::uniform_real_distribution<double> dist(0.0, total);

    double r = dist(rng);
    double acc = 0.0;

    for (size_t i = 0; i < n; i++) {
        acc += weights[i];
        if (acc >= r)
            return i;
    }

    return n - 1;
}

std::unique_ptr<SelectionFunction> RankSelection::clone() const {
    return std::make_unique<RankSelection>(*this);
}

size_t TruncationSelection::operator()(const GAsm* self) {
    static thread_local std::mt19937 rng(std::random_device{}());
    size_t n = self->populationSize;

    size_t topCount = std::max<size_t>(1, size_t((double)n * _percent));

    // Choose a random individual from the top X%
    std::uniform_int_distribution<size_t> dist(0, topCount - 1);

    // Rank-based truncation:
    // rank=0 is the best
    size_t selectedRank = dist(rng);

    // Find index with that rank
    for (size_t i = 0; i < n; i++)
        if (std::fabs(self->getRank(i) - (double)selectedRank) <= 1e-12)
            return i;

    return 0; // fallback (should never happen)
}

std::unique_ptr<SelectionFunction> TruncationSelection::clone() const {
    return std::make_unique<TruncationSelection>(*this);
}

size_t BoltzmannSelection::operator()(const GAsm* self) {
    static thread_local std::mt19937 rng(std::random_device{}());
    size_t size = self->populationSize;

    std::vector<double> weights(size);

    for (size_t i = 0; i < size; i++) {
        double f = self->getFitness(i);

        if (selectMinimal) {
            // minimize fitness → lower is better
            weights[i] = std::exp(-f / _temperature);
        } else {
            // maximize fitness → higher is better
            weights[i] = std::exp(f / _temperature);
        }
    }

    double total = std::accumulate(weights.begin(), weights.end(), 0.0);
    std::uniform_real_distribution<double> dist(0.0, total);

    double r = dist(rng);
    double acc = 0.0;

    for (size_t i = 0; i < weights.size(); i++) {
        acc += weights[i];
        if (acc >= r)
            return i;
    }

    return weights.size() - 1;
}

std::unique_ptr<SelectionFunction> BoltzmannSelection::clone() const {
    return std::make_unique<BoltzmannSelection>(*this);
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

std::unique_ptr<CrossoverFunction> OnePointCrossover::clone() const {
    return std::make_unique<OnePointCrossover>(*this);
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

std::unique_ptr<CrossoverFunction> TwoPointCrossover::clone() const {
    return std::make_unique<TwoPointCrossover>(*this);
}

void TwoPointSizeCrossover::operator()(const GAsm *self, std::vector<uint8_t> &worstIndividual,
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

    size_t newSize = std::min<size_t>(crossPoint2 - crossPoint1 + bestIndividual2.size(), self->individualMaxSize);

    worstIndividual.resize(newSize);

    std::copy_n(bestIndividual1.data(), crossPoint2, worstIndividual.data());
    std::copy_n(bestIndividual2.data() + crossPoint1, newSize - crossPoint2, worstIndividual.data() + crossPoint2);
}

std::unique_ptr<CrossoverFunction> TwoPointSizeCrossover::clone() const {
    return std::make_unique<TwoPointSizeCrossover>(*this);
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

std::unique_ptr<CrossoverFunction> UniformPointCrossover::clone() const {
    return std::make_unique<UniformPointCrossover>(*this);
}

void HardMutation::operator()(const GAsm *self, std::vector<uint8_t> &worstIndividual,
                              const std::vector<uint8_t> &bestIndividual) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> probDist(0.0, 1.0);
    std::uniform_int_distribution<int> byteDist(0, 31); // bytecode range is 0-31

    worstIndividual = bestIndividual;

    for (unsigned char& i : worstIndividual) {
        if (probDist(rng) < self->mutationProbability) {
            i = GAsmParser::base322Bytecode(byteDist(rng)); // mutate this byte
        }
    }
}

std::unique_ptr<MutationFunction> HardMutation::clone() const {
    return std::make_unique<HardMutation>(*this);
}

static std::array<std::uniform_int_distribution<int>, INSTRUCTION_GROUPS> makeDists() {
    std::array<std::uniform_int_distribution<int>, INSTRUCTION_GROUPS> arr;

    for (int i = 0; i < INSTRUCTION_GROUPS; i++) {
        arr[i] = std::uniform_int_distribution<int>(0, GAsmParser::instructionGroupLengths[i] - 1);
    }

    return arr;
}

void SoftMutation::operator()(const GAsm *self, std::vector<uint8_t> &worstIndividual,
                              const std::vector<uint8_t> &bestIndividual) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> probDist(0.0, 1.0);

    static const std::array<std::uniform_int_distribution<int>, INSTRUCTION_GROUPS> dists = makeDists();

    worstIndividual = bestIndividual;

    for (unsigned char& i : worstIndividual) {
        if (probDist(rng) < self->mutationProbability) {
            auto dist = dists[i >> 4];
            i = dist(rng) | (i & 0b11110000); // mutate the end of this byte
        }
    }
}

std::unique_ptr<MutationFunction> SoftMutation::clone() const {
    return std::make_unique<SoftMutation>(*this);
}

void FullGrow::operator()(const GAsm *self, std::vector<uint8_t> &individual) {
    individual.clear();

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 31);

    individual.resize(self->individualMaxSize);
    std::generate(individual.begin(),
                  individual.end(),
                  [&](){ return GAsmParser::base322Bytecode(dist(engine));}
    );
}

std::unique_ptr<GrowFunction> FullGrow::clone() const {
    return std::make_unique<FullGrow>(*this);
}

void SizeGrow::operator()(const GAsm *self, std::vector<uint8_t> &individual) {
    individual.clear();

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 31);

    individual.resize(size_);
    std::generate_n(individual.begin(),
                    size_,
                    [&](){ return GAsmParser::base322Bytecode(dist(engine));}
    );
}

std::unique_ptr<GrowFunction> SizeGrow::clone() const {
    return std::make_unique<SizeGrow>(*this);
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

            if (individual.size() + 2 >= maxSize) { // space for 2 additional instructions
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

std::unique_ptr<GrowFunction> TreeGrow::clone() const {
    return std::make_unique<TreeGrow>(*this);
}
