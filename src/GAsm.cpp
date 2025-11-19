//
// Created by mateu on 14.11.2025.
//

#include <filesystem>
#include <fstream>
#include "../include/GAsm.h"
#include "../include/GAsmParser.h"
#include <nlohmann/json.hpp>
#include <random>
#include <iostream>
#include <thread>
#include <cfloat>

GAsm::GAsm() = default;

GAsm::GAsm(const std::string &filename) {
    using nlohmann::json;
    // Read file
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open JSON file: " + filename);

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    json j = json::parse(content);

    // Load primitive fields
    populationSize      = j["populationSize"];
    individualMaxSize   = j["individualMaxSize"];
    mutationProbability = j["mutationProbability"];
    crossoverProbability= j["crossoverProbability"];
    tournamentSize      = j["tournamentSize"];
    maxGenerations      = j["maxGenerations"];
    goalFitness         = j["goalFitness"];

    // Load inputs & targets
    inputs  = j["inputs"].get<std::vector<std::vector<double>>>();
    targets = j["targets"].get<std::vector<std::vector<double>>>();

    // Load fitness & rank
    _fitness = j["fitness"].get<std::vector<double>>();
    _rank    = j["rank"].get<std::vector<double>>();

    // Load best individual
    std::string ascii = j["bestIndividual"].get<std::string>();
    size_t len = ascii.size();
    uint8_t *bytecode = GAsmParser::ascii2Bytecode(ascii, len);
    std::vector<uint8_t> individual(bytecode, bytecode + len);
    delete[] bytecode;
    bestIndividual = std::move(individual);

    // Deserialize population
    _population.clear();
    for (auto &ascii_value : j["population"])
    {
        ascii = ascii_value.get<std::string>();

        len = ascii.size();
        bytecode = GAsmParser::ascii2Bytecode(ascii, len);

        individual = std::vector<uint8_t>(bytecode, bytecode + len);
        delete[] bytecode;

        _population.push_back(std::move(individual));
    }

    // Load history
    if (j.contains("history"))
        hist = Hist(j["history"]);
    else
        hist = Hist();   // empty history
}


GAsm::~GAsm() = default;

void GAsm::save2File(const std::string& filename) {
    using nlohmann::json;
    json j;

    // Save simple fields
    j["populationSize"]    = populationSize;
    j["individualMaxSize"] = individualMaxSize;
    j["mutationProbability"] = mutationProbability;
    j["crossoverProbability"] = crossoverProbability;
    j["tournamentSize"] = tournamentSize;
    j["maxGenerations"] = maxGenerations;
    j["goalFitness"] = goalFitness;

    // Save inputs
    j["inputs"] = inputs;
    j["targets"] = targets;

    // Save bestIndividual
    std::string ascii = GAsmParser::bytecode2Ascii(
            bestIndividual.data(),
            bestIndividual.size()
    );
    j["bestIndividual"] = ascii;

    // Save population (as ASCII)
    j["population"] = json::array();
    for (const auto &individual : _population)
    {
        ascii = GAsmParser::bytecode2Ascii(
                individual.data(),
                individual.size()
        );

        j["population"].push_back(ascii);
    }

    // Save fitness & rank
    j["fitness"] = _fitness;
    j["rank"]    = _rank;

    // Save history
    j["history"] = hist.toJson();

    // Write to file
    std::ofstream f(filename);
    f << j.dump(4);   // pretty print
    f.close();
}

static void printHeader(const GAsm* const self) {
    std::cout << "-- GAsm evolution --" << std::endl;
    std::cout << "Seed: " << self->seed << std::endl;
    std::cout << "Max individual size: " << self->individualMaxSize << std::endl;
    std::cout << "Population size: " << self->populationSize << std::endl;
    std::cout << "Crossover probability: " << self->crossoverProbability << std::endl;
    std::cout << "Mutation probability: " << self->mutationProbability << std::endl;
    std::cout << "Max generations:" << self->maxGenerations << std::endl;
    std::cout << "Tournament size: " << self->tournamentSize << std::endl;
    std::cout << "----------------------------------" << std::endl;
}

double GAsm::printGenerationStats(int generation) {
    double avgFitness = 0.0;
    double bestFitness = -DBL_MAX; // NOLINT
    double avgSize = 0.0;
    size_t bestIndividualIndex = 0;
    for (int i = 0; i < _population.size(); i++) {
        avgFitness += _fitness[i];
        avgSize += (double)_population[i].size();
        if (_fitness[i] > bestFitness) {
            bestFitness = _fitness[i];
            bestIndividualIndex = i;
        }
    }
    avgFitness /= (double)_population.size();
    avgSize /= (double)_population.size();
    bestIndividual = _population[bestIndividualIndex];
    hist.add(generation, bestFitness, avgFitness, avgSize, bestIndividual);
    std::cout << "Generation: " << generation
              << ", Avg Fitness: " << avgFitness
              << ", Best Fitness: " << bestFitness
              << ", Avg Size: " << avgSize << std::endl;
    return bestFitness;
}

static void printTime(double elapsedSeconds) {
    int minutes = (int)elapsedSeconds / 60;
    int hours = minutes / 60;
    elapsedSeconds -= minutes * 60;
    minutes -= hours * 60;
    if (hours != 0) std::cout << hours << "h ";
    if (minutes != 0) {
        std::cout << minutes << "m ";
        std::cout << (int)elapsedSeconds << "s";
    } else {
        std::cout << std::fixed
                  << std::setprecision(2)
                  << elapsedSeconds << "s";
    }
}

static void printProgressBar(int progressPercent, double elapsedSeconds) {
    const int barWidth = 100;
    int filled = progressPercent;

    std::cout << "\r[";
    for (int i = 0; i < filled; ++i) std::cout << "■";
    for (int i = filled; i < barWidth; ++i) std::cout << " ";
    std::cout << "] ";
    printTime(elapsedSeconds);
    std::cout << std::flush;
}

void GAsm::evolve(const std::vector<std::vector<double>>& inputs,
                  const std::vector<std::vector<double>>& targets)
{
    using namespace std::chrono;
    // TODO std::move inputs and targets
    this->inputs = inputs;
    this->targets = targets;

    // Resize population and fitness
    _population.resize(populationSize);
    _fitness.resize(populationSize);
    _rank.resize(populationSize, 0.0);
    for (auto& ind : _population) {
        ind.reserve(individualMaxSize);
    }

    printHeader(this);

    std::cout << "Initializing population" << std::endl;
    auto initStart = high_resolution_clock::now();
    for (size_t i = 0; i < populationSize; i++) {
        int progress = ((int)i + 1) * 100 / (int) populationSize;
        double elapsed = duration<double>(high_resolution_clock::now() - initStart).count();
        printProgressBar(progress, elapsed);
        growFunction(this, _population[i]);
        _fitness[i] = fitnessFunction(this, _population[i]);
    }
    std::cout << std::endl;
    printGenerationStats(0);

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<double> dist(0, 1);

    auto evolutionStart = high_resolution_clock::now();

    for (int generation = 0; generation < maxGenerations; generation++) {
        auto genStart = high_resolution_clock::now();
        for (int i = 0; i < populationSize; i++) {
            size_t worstIndex = negativeSelectionFunction(this);
            if (dist(engine) < crossoverProbability) {
                size_t bestIndex1 = selectionFunction(this);
                size_t bestIndex2 = selectionFunction(this);

                crossoverFunction(this, _population[worstIndex], _population[bestIndex1], _population[bestIndex2]);
            } else {
                size_t bestIndex = selectionFunction(this);
                mutationFunction(this, _population[worstIndex], _population[bestIndex]);
            }

            _fitness[worstIndex] = fitnessFunction(this, _population[worstIndex]);
            int progress = (i + 1) * 100 / (int) populationSize;
            double elapsed = duration<double>(high_resolution_clock::now() - genStart).count();
            printProgressBar(progress, elapsed);
        }
        std::cout << std::endl;

        double fitness = printGenerationStats(generation + 1);

        // Early stopping
        if (fitness > goalFitness) break;
    }
    double elapsed = duration<double>(high_resolution_clock::now() - evolutionStart).count();
    std::cout << "Evolution finished, took: ";
    printTime(elapsed);
    std::cout << std::endl;
}

void GAsm::fullGrow(const GAsm *self, std::vector<uint8_t> &individual) {
    individual.clear();

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<uint8_t> dist(0, 31);

    individual.resize(self->individualMaxSize);
    std::generate(individual.begin(),
                  individual.end(),
                  [&](){ return GAsmParser::base322Bytecode(dist(engine)); });
}

double GAsm::fitness(const GAsm *self, const std::vector<uint8_t> &individual) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double score = 0.0;
    for (unsigned char i : individual) {
        if (i == 0x00) score++;
    }
    return score;
}

size_t GAsm::tournamentSelection(const GAsm *self) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, self->_population.size() - 1);

    size_t bestIndex = dist(rng);
    for (unsigned int i = 1; i < self->tournamentSize; i++) {
        size_t idx = dist(rng);
        if (self->_fitness[idx] > self->_fitness[bestIndex]) {
            bestIndex = idx;
        }
    }
    return bestIndex;
}

size_t GAsm::negativeTournamentSelection(const GAsm *self) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, self->_population.size() - 1);

    size_t worstIndex = dist(rng);
    for (unsigned int i = 1; i < self->tournamentSize; i++) {
        size_t idx = dist(rng);
        if (self->_fitness[idx] < self->_fitness[worstIndex]) {
            worstIndex = idx;
        }
    }

    return worstIndex;
}

void GAsm::onePointCrossover(const GAsm *self, std::vector<uint8_t> &worstIndividual,
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

void GAsm::twoPointCrossover(const GAsm *self, std::vector<uint8_t> &worstIndividual,
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

void GAsm::uniformCrossover(const GAsm *self, std::vector<uint8_t> &worstIndividual,
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

void GAsm::hardMutation(const GAsm *self, std::vector<uint8_t>& worstIndividual,
                        const std::vector<uint8_t>& bestIndividual) {
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

void GAsm::softMutation(const GAsm *self, std::vector<uint8_t>& worstIndividual,
                        const std::vector<uint8_t>& bestIndividual) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> probDist(0.0, 1.0);

    // TODO do it dynamically based on GAsmParser::_string2Opcode
    static const std::uniform_int_distribution<uint8_t> dists[GAsmParser::instructionGroups] = {
            std::uniform_int_distribution<uint8_t>(0, 6),
            std::uniform_int_distribution<uint8_t>(0, 7),
            std::uniform_int_distribution<uint8_t>(0, 7),
            std::uniform_int_distribution<uint8_t>(0, 4),
            std::uniform_int_distribution<uint8_t>(0, 3),
            std::uniform_int_distribution<uint8_t>(0, 3),
            std::uniform_int_distribution<uint8_t>(0, 2),
    };

    worstIndividual = bestIndividual;

    for (unsigned char& i : worstIndividual) {
        if (probDist(rng) < self->mutationProbability) {
            auto dist = dists[i >> 4];
            i = dist(rng) | (i & 0b11110000); // mutate the end of this byte
        }
    }
}
