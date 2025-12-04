//
// Main class, evolves the population, evaluates it
//

#ifndef GASM_GASM_H
#define GASM_GASM_H


unsigned int operator ""_pop_size(unsigned long long int);
unsigned int operator ""_ind_size(unsigned long long int);
double operator ""_p_mut(long double);
double operator ""_p_cross(long double);
unsigned int operator ""_t_size(unsigned long long int);
unsigned int operator ""_max_gen(unsigned long long int);

#include <cstdint>
#include <vector>
#include <functional>
#include <string>
#include <cfloat>
#include <thread>
#include "Hist.h"
#include "functions.h"
#include "GAsmInterpreter.h"

using gen_fun_t = double(*)();

class GAsm {
private:
    std::vector<std::vector<uint8_t>> _population;
    std::vector<double> _fitness;
    std::vector<double> _rank;  // other parameter determining the quality of individual

    GAsmInterpreter _runner;
    size_t _registerLength = 10;
    gen_fun_t cng = nullptr;
    gen_fun_t rng = nullptr;
    bool _useCompile = true;

    double printGenerationStats(int gen);
public:
    // getters and setters
    [[nodiscard]] const std::vector<std::vector<uint8_t>>& getPopulation() const;
    [[nodiscard]] const std::vector<double>& getFitness() const;
    [[nodiscard]] const std::vector<double>& getRank() const;
    [[nodiscard]] const double& getFitness(size_t i) const;
    [[nodiscard]] const double& getRank(size_t i) const;
    [[nodiscard]] const gen_fun_t& getCNG() const;
    void setCNG(const gen_fun_t& cng_);
    [[nodiscard]] const gen_fun_t& getRNG() const;
    void setRNG(const gen_fun_t& rng_);
    [[nodiscard]] const bool& getCompile() const;
    void setCompile(const bool& useCompile);

    // public attributes
    unsigned int populationSize = 1000;
    unsigned int individualMaxSize = 30;
    double mutationProbability = 0.05;
    double crossoverProbability = 0.9;
    unsigned int maxGenerations = 5;
    double goalFitness = 1000.0;
    bool minimize = false;
    Hist hist = Hist();
    std::vector<std::vector<double>> inputs;
    std::vector<std::vector<double>> targets;
    std::vector<uint8_t> bestIndividual;

    // public runner attributes
    [[nodiscard]] size_t getRegisterLength() const {return _registerLength;}
    void setRegisterLength(const size_t& length) {_registerLength = length; _runner.setRegisterLength(length);}
    size_t maxProcessTime = 1000;

    // constructors
    GAsm();
    explicit GAsm(const std::string& filename);

    // screw C++
    GAsm(const GAsm& other) = delete;
    GAsm& operator=(const GAsm& other) = delete;
    GAsm(GAsm&& other) = delete;
    GAsm& operator=(GAsm&& other) = delete;
    ~GAsm();

    // progress saving methods
//    void makeCheckpoint(); TODO
    void save2File(const std::string& filename);

    // evolution methods and attributes
    void setProgram(const std::vector<uint8_t> &program);
    size_t run(std::vector<double>& inputs);
    double runAll(const std::vector<uint8_t> &program, std::vector<std::vector<double>> &outputs);
    void evolve(const std::vector<std::vector<double>>& inputs, const std::vector<std::vector<double>>& targets);
    // TODO free these pointers (setters and getters)
    FitnessFunction* fitnessFunction = new Fitness();
    SelectionFunction* selectionFunction = new TournamentSelection(2);
    CrossoverFunction* crossoverFunction = new OnePointCrossover();
    MutationFunction* mutationFunction = new HardMutation();
    GrowFunction* growFunction = new FullGrow();

//    std::unique_ptr<FitnessFunction> fitnessFunction = std::make_unique<Fitness>();
//    std::unique_ptr<SelectionFunction> selectionFunction = std::make_unique<TournamentSelection>(2);
//    std::unique_ptr<CrossoverFunction> crossoverFunction = std::make_unique<OnePointCrossover>();
//    std::unique_ptr<MutationFunction> mutationFunction = std::make_unique<HardMutation>();
//    std::unique_ptr<GrowFunction> growFunction = std::make_unique<FullGrow>();

//    void setFitnessFunction(std::unique_ptr<FitnessFunction> f) {
//        fitnessFunction_ = std::move(f);
//    }
//
//    void setSelectionFunction(std::unique_ptr<SelectionFunction> s) {
//        selectionFunction_ = std::move(s);
//    }
//
//    void setCrossoverFunction(std::unique_ptr<CrossoverFunction> c) {
//        crossoverFunction_ = std::move(c);
//    }
//
//    void setMutationFunction(std::unique_ptr<MutationFunction> m) {
//        mutationFunction_ = std::move(m);
//    }
//
//    void setGrowFunction(std::unique_ptr<GrowFunction> g) {
//        growFunction_ = std::move(g);
//    }
//
//    FitnessFunction& fitness() { return *fitnessFunction_; }
//    SelectionFunction& selection() { return *selectionFunction_; }
//    CrossoverFunction& crossover() { return *crossoverFunction_; }
//    MutationFunction& mutation() { return *mutationFunction_; }
//    GrowFunction& grow() { return *growFunction_; }
};


#endif //GASM_GASM_H
