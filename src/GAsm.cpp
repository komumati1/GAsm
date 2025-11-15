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

GAsm::GAsm() = default;

GAsm::GAsm(const std::string &filename)
{
    using nlohmann::json;
    // Read file
    auto size = std::filesystem::file_size(filename);
    std::string content(size, '\0');
    std::ifstream file(filename);
    file.read(&content[0], (long long)size);
    file.close();

    json j = json::parse(content);

    // Load primitive fields
    populationSize      = j["populationSize"];
    individualMaxSize   = j["individualMaxSize"];
    mutationProbability = j["mutationProbability"];
    crossoverProbability = j["crossoverProbability"];
    tournamentSize      = j["tournamentSize"];
    maxGenerations      = j["maxGenerations"];

    // Load inputs & targets
    inputs  = j["inputs"].get<std::vector<std::vector<double>>>();
    targets = j["targets"].get<std::vector<std::vector<double>>>();

    // Load fitness & rank
    _fitness = j["fitness"].get<std::vector<double>>();
    _rank    = j["rank"].get<std::vector<double>>();

    // Deserialize population
    _population.clear();
    for (auto &ascii_value : j["population"])
    {
        std::string ascii = ascii_value.get<std::string>();

        size_t len = ascii.size();
        uint8_t *bytecode = GAsmParser::ascii2Bytecode(ascii, len);

        std::vector<uint8_t> individual(bytecode, bytecode + len);
        delete[] bytecode;

        _population.push_back(std::move(individual));
    }
}


GAsm::~GAsm() = default;

void GAsm::save2File(const std::string& filename)
{
    using nlohmann::json;
    json j;

    // Save simple fields
    j["populationSize"]    = populationSize;
    j["individualMaxSize"] = individualMaxSize;
    j["mutationProbability"] = mutationProbability;
    j["crossoverProbability"] = crossoverProbability;
    j["tournamentSize"] = tournamentSize;
    j["maxGenerations"] = maxGenerations;

    // Save inputs
    j["inputs"] = inputs;
    j["targets"] = targets;

    // Save population (as ASCII)
    j["population"] = json::array();
    for (const auto &individual : _population)
    {
        std::string ascii = GAsmParser::bytecode2Ascii(
                individual.data(),
                individual.size()
        );

        j["population"].push_back(ascii);
    }

    // Save fitness & rank
    j["fitness"] = _fitness;
    j["rank"]    = _rank;

    // Write to file
    std::ofstream f(filename);
    f << j.dump(4);   // pretty print
    f.close();
}

static void printProgressBar(int progressPercent, double elapsedSeconds) {
    const int barWidth = 100;
    int filled = progressPercent;

    std::cout << "\r[";
    for (int i = 0; i < filled; ++i) std::cout << "■";
    for (int i = filled; i < barWidth; ++i) std::cout << " ";
    std::cout << "] ";

    std::cout << progressPercent << "%  "
              << std::fixed << std::setprecision(2)
              << elapsedSeconds << "s" << std::flush;
}

void GAsm::evolve(const std::vector<std::vector<double>>& inputs,
                  const std::vector<std::vector<double>>& targets)
{
    this->inputs = inputs;
    this->targets = targets;

    // Resize population and fitness
    _population.resize(populationSize);
    _fitness.resize(populationSize);
    _rank.resize(populationSize, 0.0);

    // Initialize population
    for (size_t i = 0; i < populationSize; ++i) {
        growFunction(this, _population[i]);
        _fitness[i] = fitnessFunction(this, _population[i]);
    }

    std::cout << "Initial population generated: " << populationSize
              << " individuals, max size " << individualMaxSize << std::endl;

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<double> dist(0, 1);

    using namespace std::chrono;
    auto evolutionStart = high_resolution_clock::now();

    for (int generation = 0; generation < maxGenerations; generation++) {
        auto genStart = high_resolution_clock::now();
        for (int i = 0; i < populationSize; i++) {
            size_t worstIndex = negativeTournamentSelection(this);
            if (dist(engine) < crossoverProbability) {
                size_t bestIndex1 = tournamentSelection(this);
                size_t bestIndex2 = tournamentSelection(this);

                dumbCrossover(this, _population[worstIndex], _population[bestIndex1], _population[bestIndex2]);
            } else {
                size_t bestIndex = tournamentSelection(this);
                dumbMutation(this, _population[worstIndex], _population[bestIndex]);
            }

            _fitness[worstIndex] = fitnessFunction(this, _population[worstIndex]);
            int progress = (i + 1) * 100 / (int) populationSize;
            double elapsed = duration<double>(high_resolution_clock::now() - genStart).count();
            printProgressBar(progress, elapsed);
        }
        std::cout << std::endl;
    }
    double elapsed = duration<double>(high_resolution_clock::now() - evolutionStart).count();
    std::cout << "Evolve finished took: "
              << std::fixed << std::setprecision(2)
              << elapsed << "s" << std::endl;
}

void GAsm::fullGrow(GAsm *self, std::vector<uint8_t> &individual) {
    individual.clear();

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<uint8_t> dist(0, 31);

    individual.resize(self->individualMaxSize);
    std::generate(individual.begin(),
                  individual.end(),
                  [&](){ return GAsmParser::base322Bytecode(dist(engine)); });
}

double GAsm::fitness(GAsm *self, const std::vector<uint8_t> &individual) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double score = 0.0;
    for (unsigned char i : individual) {
        if (i == 0x00) score++;
    }
    return score;
}

size_t GAsm::tournamentSelection(GAsm *self) {
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

size_t GAsm::negativeTournamentSelection(GAsm *self) {
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

void GAsm::dumbCrossover(GAsm *self, std::vector<uint8_t> &worstIndividual,
                         const std::vector<uint8_t> &bestIndividual1,
                         const std::vector<uint8_t> &bestIndividual2) {
    if (bestIndividual1.empty() || bestIndividual2.empty()) return;

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, bestIndividual1.size() - 1);

    int crossPoint = (int)dist(rng);

    worstIndividual.clear();

    worstIndividual.insert(worstIndividual.begin(), bestIndividual1.begin(), bestIndividual1.begin() + crossPoint);

    worstIndividual.insert(worstIndividual.begin() + crossPoint, bestIndividual2.begin() + crossPoint, bestIndividual2.end());
}

void GAsm::dumbMutation(GAsm *self, std::vector<uint8_t> &worstIndividual,
                        const std::vector<uint8_t> &bestIndividual) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> probDist(0.0, 1.0);
    std::uniform_int_distribution<uint8_t> byteDist(0, 31); // assuming your bytecode range is 0-31

    worstIndividual = bestIndividual;

    for (unsigned char & i : worstIndividual) {
        if (probDist(rng) < self->mutationProbability) {
            i = GAsmParser::base322Bytecode(byteDist(rng)); // mutate this byte
        }
    }
}

