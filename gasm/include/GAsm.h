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
#include "Runner.h"
#include "Individual.h"

class GAsm {
private:
    std::vector<std::vector<uint8_t>> population_;
    std::vector<double> fitness_;
    std::vector<double> rank_;  // other parameter determining the quality of individual
    std::vector<std::unique_ptr<std::mutex>> individualMutexes_;

    GAsmInterpreter runner_;

    std::unique_ptr<FitnessFunction> fitnessFunction_ = std::make_unique<Fitness>();
    std::unique_ptr<SelectionFunction> selectionFunction_ = std::make_unique<TournamentSelection>(2);
    std::unique_ptr<CrossoverFunction> crossoverFunction_ = std::make_unique<OnePointCrossover>();
    std::unique_ptr<MutationFunction> mutationFunction_ = std::make_unique<HardMutation>();
    std::unique_ptr<GrowFunction> growFunction_ = std::make_unique<FullGrow>();

    std::vector<Runner> runners_;

    double printGenerationStats(int gen, bool save = true);
    void printHeader(const GAsm* self);
public:
    friend class Runner;
    // getters and setters
//    [[nodiscard]] const std::vector<std::vector<uint8_t>>& getPopulation() const { return population_; }
//    [[nodiscard]] const std::vector<double>& getFitness() const { return fitness_; }
//    [[nodiscard]] const std::vector<double>& getRank() const { return rank_; }
//    [[nodiscard]] const double& getFitness(size_t i) const { return fitness_[i]; }
//    [[nodiscard]] const double& getRank(size_t i) const { return rank_[i]; }
    [[nodiscard]] const FitnessFunction& fitness() const { return *fitnessFunction_; }
    void setFitnessFunction(std::unique_ptr<FitnessFunction> f) { fitnessFunction_ = std::move(f);
        std::for_each(runners_.begin(), runners_.end(), [this](Runner& r){ r.setFitnessFunction(fitnessFunction_->clone()); }); }
    [[nodiscard]] const SelectionFunction& selection() const { return *selectionFunction_; }
    void setSelectionFunction(std::unique_ptr<SelectionFunction> s) { selectionFunction_ = std::move(s);
        std::for_each(runners_.begin(), runners_.end(), [this](Runner& r){ r.setSelectionFunction(selectionFunction_->clone()); }); }
    [[nodiscard]] const CrossoverFunction& crossover() const { return *crossoverFunction_; }
    void setCrossoverFunction(std::unique_ptr<CrossoverFunction> c) { crossoverFunction_ = std::move(c);
        std::for_each(runners_.begin(), runners_.end(), [this](Runner& r){ r.setCrossoverFunction(crossoverFunction_->clone()); }); }
    [[nodiscard]] const MutationFunction& mutation() const { return *mutationFunction_; }
    void setMutationFunction(std::unique_ptr<MutationFunction> m) { mutationFunction_ = std::move(m);
        std::for_each(runners_.begin(), runners_.end(), [this](Runner& r){ r.setMutationFunction(mutationFunction_->clone()); }); }
    [[nodiscard]] const GrowFunction& grow() const { return *growFunction_; }
    void setGrowFunction(std::unique_ptr<GrowFunction> g) { growFunction_ = std::move(g);
        std::for_each(runners_.begin(), runners_.end(), [this](Runner& r){ r.setGrowFunction(growFunction_->clone()); }); }
    [[nodiscard]] Individual getBestIndividual() const { return Individual(bestIndividual); }

    // thread safe setters and getters
    std::vector<uint8_t> getIndividual(size_t idx) const {
        std::lock_guard<std::mutex> guard(const_cast<std::mutex&>(*individualMutexes_[idx]));
        return population_[idx];
    }
    void setIndividual(size_t idx, const std::vector<uint8_t>& bytecode, double newFitness, double newRank) {
        std::lock_guard<std::mutex> guard(*individualMutexes_[idx]);
        population_[idx] = bytecode;
        fitness_[idx] = newFitness;
        rank_[idx] = newRank;
    }

    [[nodiscard]] double getFitness(size_t idx) const {
        std::lock_guard<std::mutex> guard(const_cast<std::mutex&>(*individualMutexes_[idx]));
        return fitness_[idx];
    }

    [[nodiscard]] double getRank(size_t idx) const {
        std::lock_guard<std::mutex> guard(const_cast<std::mutex&>(*individualMutexes_[idx]));
        return rank_[idx];
    }

    [[nodiscard]] std::pair<double, double> getFitnessRankSafe(size_t idx) const {
        std::lock_guard<std::mutex> guard(const_cast<std::mutex&>(*individualMutexes_[idx]));
        return {fitness_[idx], rank_[idx]};
    }
    void setFitnessRank(size_t idx, double f, double r) {
        std::lock_guard<std::mutex> guard(*individualMutexes_[idx]);
        fitness_[idx] = f;
        rank_[idx] = r;
    }

    // public attributes
    unsigned int populationSize = 1000;
    unsigned int individualMaxSize = 30;
    double mutationProbability = 0.05;
    double crossoverProbability = 0.9;
    unsigned int maxGenerations = 5;
    double goalFitness = 1000.0;
    double nanPenalty = 1e1;
    bool minimize = false;
    std::string outputFolder;
    size_t checkPointInterval = 10;
    Hist hist = Hist();
    std::vector<std::vector<double>> inputs;
    std::vector<std::vector<double>> targets;
    std::vector<uint8_t> bestIndividual;

    // runner setters and getters
    [[nodiscard]] size_t getRegisterLength() const { return runner_.getRegisterLength(); }
    void setRegisterLength(const size_t& length) { runner_.setRegisterLength(length);
        std::for_each(runners_.begin(), runners_.end(), [length](Runner& r){ r.jit_.setRegisterLength(length); }); }
    [[nodiscard]] const gen_fn_t& getCNG() const { return runner_.getCng(); }
    void setCNG(std::unique_ptr<gen_fn_t> cng) { runner_.setCng(std::make_unique<gen_fn_t>(*cng));
        std::for_each(runners_.begin(), runners_.end(), [&cng](Runner& r){ r.jit_.setCng(std::make_unique<gen_fn_t>(*cng)); }); }
    [[nodiscard]] const gen_fn_t& getRNG() const { return runner_.getRng(); }
    void setRNG(std::unique_ptr<gen_fn_t> rng) { runner_.setRng(std::make_unique<gen_fn_t>(*rng));
        std::for_each(runners_.begin(), runners_.end(), [&rng](Runner& r){ r.jit_.setCng(std::make_unique<gen_fn_t>(*rng)); }); }
    [[nodiscard]] const bool& getCompile() const { return runner_.useCompile; }
    void setCompile(const bool& useCompile) { runner_.useCompile = useCompile;
        std::for_each(runners_.begin(), runners_.end(), [&useCompile](Runner& r){ r.jit_.useCompile = useCompile; }); }

    // public runner attributes
    size_t maxProcessTime = 10000;

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
    nlohmann::json toJson();
    void makeCheckpoint();
    void save2File(const std::string& filename);

    // evolution methods and attributes
    void setProgram(const std::vector<uint8_t> &program);
    size_t run(std::vector<double>& inputs);
    double runAll(const std::vector<uint8_t> &program, std::vector<std::vector<double>> &outputs);
    void evolve(const std::vector<std::vector<double>>& inputs, const std::vector<std::vector<double>>& targets);
    void parallelEvolve(const std::vector<std::vector<double>>& inputs_, const std::vector<std::vector<double>>& targets_);
};


#endif //GASM_GASM_H
