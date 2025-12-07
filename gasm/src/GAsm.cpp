//
// Created by mateu on 14.11.2025.
//

#include <filesystem>
#include <fstream>
#include "utils.h"
#include "../include/GAsm.h"
#include <nlohmann/json.hpp>
#include <random>
#include <iostream>
#include <thread>
#include <cfloat>
#include <boost/multiprecision/cpp_bin_float.hpp>

GAsm::GAsm() : runner_(1), population_(0), fitness_(1), rank_(1) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // fallback
    runners_.reserve(numThreads);

    for (unsigned int i = 0; i < numThreads; i++) {
        runners_.emplace_back(std::move(Runner()));
    }
}

GAsm::GAsm(const std::string &filename) : runner_(1) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // fallback
    runners_.reserve(numThreads);

    for (unsigned int i = 0; i < numThreads; i++) {
        runners_.emplace_back(std::move(Runner()));
    }
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
    maxProcessTime      = j["maxProcessTime"];
    outputFolder        = j["outputFolder"];
    checkPointInterval  = j["checkPointInterval"];

    // Load register length
    setRegisterLength(j["registerLength"]);

    // Load inputs & targets
    inputs  = j["inputs"].get<std::vector<std::vector<double>>>();
    targets = j["targets"].get<std::vector<std::vector<double>>>();

    // Load fitness & rank
    fitness_ = j["fitness"].get<std::vector<double>>();
    rank_    = j["rank"].get<std::vector<double>>();

    // Load best individual
    std::string ascii = j["bestIndividual"].get<std::string>();
    size_t len = ascii.size();
    uint8_t *bytecode = GAsmParser::ascii2Bytecode(ascii, len);
    std::vector<uint8_t> individual(bytecode, bytecode + len);
    delete[] bytecode;
    bestIndividual = std::move(individual);

    // Deserialize population
    population_.clear();
    for (auto &ascii_value : j["population"])
    {
        ascii = ascii_value.get<std::string>();

        len = ascii.size();
        bytecode = GAsmParser::ascii2Bytecode(ascii, len);

        individual = std::vector<uint8_t>(bytecode, bytecode + len);
        delete[] bytecode;

        population_.push_back(std::move(individual));
    }

    // Load history
    if (j.contains("history"))
        hist = Hist(j["history"]);
    else
        hist = Hist();   // empty history
}

GAsm::~GAsm() = default;

nlohmann::json GAsm::toJson() {
    using nlohmann::json;
    json j;
    // Save simple fields
    j["populationSize"]    = populationSize;
    j["individualMaxSize"] = individualMaxSize;
    j["mutationProbability"] = mutationProbability;
    j["crossoverProbability"] = crossoverProbability;
    j["maxGenerations"] = maxGenerations;
    j["goalFitness"] = goalFitness;
    j["outputFolder"] = outputFolder;
    j["checkPointInterval"] = checkPointInterval;
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
    for (const auto &individual : population_)
    {
        ascii = GAsmParser::bytecode2Ascii(
                individual.data(),
                individual.size()
        );

        j["population"].push_back(ascii);
    }

    // Save fitness & rank
    j["fitness"] = fitness_;
    j["rank"]    = rank_;

    // Save history
    j["history"] = hist.toJson();

    // Save register length
    j["registerLength"] = runner_.getRegisterLength();

    return j;
}

void GAsm::save2File(const std::string& filename) {
    // Write to file
    std::ofstream f(filename);
    if (!f.is_open()) {
        std::cerr << "FAILED to open file: " << filename << "\n";
        return;
    }
    std::cout << "Writing to file: " << std::filesystem::absolute(filename) << std::endl;
    f << toJson().dump(4);   // pretty print
    f.close();
    std::cout << "File written successfully!" << std::endl;
}

void GAsm::printHeader(const GAsm* const self) {
    std::cout << "-- GAsm evolution --" << std::endl;
    std::cout << "Max individual size: " << self->individualMaxSize << std::endl;
    std::cout << "Population size: " << self->populationSize << std::endl;
    std::cout << "Crossover probability: " << self->crossoverProbability << std::endl;
    std::cout << "Mutation probability: " << self->mutationProbability << std::endl;
    std::cout << "Max generations: " << self->maxGenerations << std::endl;
    std::cout << "Register length: " << self->runner_.getRegisterLength() << std::endl;
    std::cout << "Max process time: " << self->maxProcessTime << std::endl;
    std::cout << "Minimize: " << (self->minimize ? "True" : "False") << std::endl;
    std::cout << "Output folder: " << self->outputFolder << std::endl;
    std::cout << "Checkpoint interval: " << self->checkPointInterval << std::endl;
    std::cout << "Goal fitness: " << self->goalFitness << std::endl;
    std::cout << "NaN penalty: " << self->nanPenalty << std::endl;
    std::cout << "Number of cores: " << self->runners_.size() << std::endl;
    std::cout << "----------------------------------" << std::endl;
}

double GAsm::printGenerationStats(int generation) {
    boost::multiprecision::cpp_bin_float_quad avgFitness = 0.0;
    double bestFitness = minimize ? DBL_MAX: -DBL_MAX; // NOLINT
    double avgSize = 0.0;
    size_t bestIndividualIndex = 0;
    for (int i = 0; i < population_.size(); i++) {
        avgFitness += std::isfinite(fitness_[i]) ? fitness_[i] : 0;
        avgSize += (double)population_[i].size();
        if (minimize ? fitness_[i] < bestFitness : fitness_[i] > bestFitness) {
            bestFitness = fitness_[i];
            bestIndividualIndex = i;
        }
    }
    avgFitness /= (double)population_.size();
    avgSize /= (double)population_.size();
    bestIndividual = population_[bestIndividualIndex];
    auto convertedAvgFitness = (double)avgFitness;
    hist.add(generation, bestFitness, convertedAvgFitness, avgSize, bestIndividual);
    std::cout << "Generation: " << generation
              << ", Avg Fitness: " << std::scientific << convertedAvgFitness
              << ", Best Fitness: " << std::fixed << std::setprecision(2) << bestFitness
              << ", Avg Size: " << avgSize << std::endl;
    return bestFitness;
}

void GAsm::parallelEvolve(const std::vector<std::vector<double>>& inputs_,
                          const std::vector<std::vector<double>>& targets_)
{
    using namespace std::chrono;
    // TODO checkpoints
    // TODO from checkpoint
    this->inputs = inputs_;
    this->targets = targets_;

    // Resize population and fitness
    population_.resize(populationSize);
    fitness_.resize(populationSize);
    rank_.resize(populationSize);
    individualMutexes_.resize(populationSize);
    for (auto& m : individualMutexes_)
        m = std::make_unique<std::mutex>();
    for (auto& ind : population_) {
        ind.reserve(individualMaxSize);
    }
    size_t numThreads = runners_.size();
    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    size_t chunk = (populationSize + numThreads - 1) / numThreads;

    printHeader(this);

    std::cout << "Initializing population" << std::endl;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * chunk;
        size_t end = std::min<size_t>(start + chunk, populationSize);

        threads.emplace_back([&, t, start, end]() {
            // each runner gets its chunk and works
            runners_[t].dispatchGrow(this, start, end, t == 0);  // set first to verbose
        });
    }

    for (auto &th : threads) th.join(); // wait for all the threads
    threads.clear();
    std::cout << std::endl;
    printGenerationStats(0);

    auto evolutionStart = high_resolution_clock::now();

    for (int generation = 0; generation < maxGenerations; generation++) {
        for (size_t t = 0; t < numThreads; ++t) {
            size_t start = t * chunk;
            size_t end = std::min<size_t>(start + chunk, populationSize);

            threads.emplace_back([&, t, start, end]() {
                // each runner gets its chunk and works
                runners_[t].dispatchEvolve(this, start, end, t == 0);  // set first to verbose
            });
        }
        for (auto &th : threads) th.join(); // wait for all the threads
        threads.clear();
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

void GAsm::evolve(const std::vector<std::vector<double>>& inputs_,
                  const std::vector<std::vector<double>>& targets_)
{
    using namespace std::chrono;
    // TODO checkpoints
    // TODO from checkpoint
    this->inputs = inputs_;
    this->targets = targets_;

    // Resize population and fitness
    population_.resize(populationSize);
    fitness_.resize(populationSize);
    rank_.resize(populationSize);
    individualMutexes_.resize(populationSize);
    for (auto& m : individualMutexes_)
        m = std::make_unique<std::mutex>();
    for (auto& ind : population_) {
        ind.reserve(individualMaxSize);
    }

    printHeader(this);

    std::cout << "Initializing population" << std::endl;
    auto initStart = high_resolution_clock::now();
    for (size_t i = 0; i < populationSize; i++) {
        int progress = ((int)i + 1) * 100 / (int) populationSize;
        double elapsed = duration<double>(high_resolution_clock::now() - initStart).count();
        printProgressBar(progress, elapsed);
        (*growFunction_)(this, population_[i]);
//        std::cout << std::endl << GAsmParser::bytecode2Text(population_[i].data(), population_[i].size()) << std::endl;
        std::pair<double, double> fitRank = (*fitnessFunction_)(this, this->runner_, population_[i]);
//        std::cout << "Fitness: " << fitRank.first << std::endl;
//        std::cout << "Rank: " << fitRank.second << std::endl;
//        std::cout << "Individual: " << GAsmParser::bytecode2Text(population_[i].data(), population_[i].size()) << std::endl;
        fitness_[i] = fitRank.first;
        rank_[i] = fitRank.second;
    }
    std::cout << std::endl;
    printGenerationStats(0);

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<double> dist(0, 1);

    auto evolutionStart = high_resolution_clock::now();

    for (int generation = 0; generation < maxGenerations; generation++) {
        auto genStart = high_resolution_clock::now();
        for (int i = 0; i < populationSize; i++) {
            selectionFunction_->selectMinimal = !minimize; // worst is not minimized
            size_t worstIndex = (*selectionFunction_)(this);
            selectionFunction_->selectMinimal = minimize;  // best is minimized
            if (dist(engine) < crossoverProbability) {
                size_t bestIndex1 = (*selectionFunction_)(this);
                size_t bestIndex2 = (*selectionFunction_)(this);

                (*crossoverFunction_)(this, population_[worstIndex], population_[bestIndex1], population_[bestIndex2]);
            } else {
                size_t bestIndex = (*selectionFunction_)(this);
                (*mutationFunction_)(this, population_[worstIndex], population_[bestIndex]);
            }

            std::pair<double, double> fitRank = (*fitnessFunction_)(this, this->runner_, population_[worstIndex]);
            fitness_[worstIndex] = fitRank.first;
            rank_[worstIndex] = fitRank.second;
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

void GAsm::setProgram(const std::vector<uint8_t>& program) {
    runner_.setProgram(program);
}

size_t GAsm::run(std::vector<double>& inputs_) {
    return runner_.run(inputs_, maxProcessTime);
}

double GAsm::runAll(const std::vector<uint8_t>& program, std::vector<std::vector<double>>& outputs) {
    runner_.setProgram(program);
    double sumTime = 0.0;
    for (const auto& input : inputs) {
        std::vector<double> output = input;
        sumTime += (double)runner_.run(output, maxProcessTime);
        outputs.push_back(std::move(output));
    }
    return sumTime;
}

void GAsm::makeCheckpoint() {
    // TODO make a checkpoint
}
