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

GAsm::GAsm() : _p(std::vector<uint8_t>(0)), _runner(_p, 1) { // TODO dirty fix
    _runner.setRegisterLength(_registerLength);
}

GAsm::GAsm(const std::string &filename) : _p(std::vector<uint8_t>(0)), _runner(_p, 1) { // TODO dirty fix
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
    maxGenerations      = j["maxGenerations"];
    goalFitness         = j["goalFitness"];
    _registerLength      = j["registerLength"];
    maxProcessTime      = j["maxProcessTime"];

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

    // Set runner
    _runner.setRegisterLength(_registerLength);
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
    j["maxGenerations"] = maxGenerations;
    j["goalFitness"] = goalFitness;
    j["registerLength"] = _registerLength;
    j["maxProcessTime"] = maxProcessTime;

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
    std::cout << "Max individual size: " << self->individualMaxSize << std::endl;
    std::cout << "Population size: " << self->populationSize << std::endl;
    std::cout << "Crossover probability: " << self->crossoverProbability << std::endl;
    std::cout << "Mutation probability: " << self->mutationProbability << std::endl;
    std::cout << "Max generations:" << self->maxGenerations << std::endl;
    std::cout << "----------------------------------" << std::endl;
}

double GAsm::printGenerationStats(int generation) {
    double avgFitness = 0.0;
    double bestFitness = minimize ? DBL_MAX: -DBL_MAX; // NOLINT
    double avgSize = 0.0;
    size_t bestIndividualIndex = 0;
    for (int i = 0; i < _population.size(); i++) {
        avgFitness += _isnan(_fitness[i]) ? 0 : _fitness[i];
        avgSize += (double)_population[i].size();
        if (minimize ? _fitness[i] < bestFitness : _fitness[i] > bestFitness) {
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

void GAsm::evolve(const std::vector<std::vector<double>>& inputs_,
                  const std::vector<std::vector<double>>& targets_)
{
    using namespace std::chrono;

    this->inputs = inputs_;
    this->targets = targets_;

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
        (*growFunction)(this, _population[i]);
//        std::cout << std::endl << GAsmParser::bytecode2Text(_population[i].data(), _population[i].size()) << std::endl;
        _fitness[i] = (*fitnessFunction)(this, _population[i], i);
    }
    std::cout << std::endl;
    printGenerationStats(0);

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<double> dist(0, 1);

    auto evolutionStart = high_resolution_clock::now();

    for (int generation = 0; generation < maxGenerations; generation++) {
        auto genStart = high_resolution_clock::now();
        for (int i = 0; i < populationSize; i++) {
            selectionFunction->selectMinimal = !minimize; // worst is not minimized
            size_t worstIndex = (*selectionFunction)(this);
            selectionFunction->selectMinimal = minimize;  // best is minimized
            if (dist(engine) < crossoverProbability) {
                size_t bestIndex1 = (*selectionFunction)(this);
                size_t bestIndex2 = (*selectionFunction)(this);

                (*crossoverFunction)(this, _population[worstIndex], _population[bestIndex1], _population[bestIndex2]);
            } else {
                size_t bestIndex = (*selectionFunction)(this);
                (*mutationFunction)(this, _population[worstIndex], _population[bestIndex]);
            }

            _fitness[worstIndex] = (*fitnessFunction)(this, _population[worstIndex], worstIndex);
            int progress = (i + 1) * 100 / (int) populationSize;
            double elapsed = duration<double>(high_resolution_clock::now() - genStart).count();
            printProgressBar(progress, elapsed);
        }
        std::cout << std::endl;

        double fitness = printGenerationStats(generation + 1);

        // Early stopping
        if (minimize) {
            if (fitness <= goalFitness) break;
        } else {
            if (fitness >= goalFitness) break;
        }
    }
    double elapsed = duration<double>(high_resolution_clock::now() - evolutionStart).count();
    std::cout << "Evolution finished, took: ";
    printTime(elapsed);
    std::cout << std::endl;
}

const std::vector<std::vector<uint8_t>> &GAsm::getPopulation() const {
    return _population;
}

const std::vector<double> &GAsm::getFitness() const {
    return _fitness;
}

const std::vector<double> &GAsm::getRank() const {
    return _rank;
}

void GAsm::setProgram(std::vector<uint8_t>& program) {
    _runner.useCompile = useCompile;
    _runner.setProgram(program);
}

size_t GAsm::run(std::vector<double> &inputs_) {
    return _runner.run(inputs_, maxProcessTime);
}
