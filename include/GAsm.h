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

class GAsm {
private:
    std::vector<std::vector<uint8_t>> _population;
    std::vector<double> _fitness;
    std::vector<double> _rank;  // other parameter determining the quality of individual

    double printGenerationStats(int gen);
public:
    [[nodiscard]] const std::vector<std::vector<uint8_t>> &getPopulation() const;
    [[nodiscard]] const std::vector<double> &getFitness() const;
    [[nodiscard]] const std::vector<double> &getRank() const;

    unsigned int populationSize = 100000;
    unsigned int individualMaxSize = 10000;
    double mutationProbability = 0.05;
    double crossoverProbability = 0.9;
    unsigned int maxGenerations = 100;
    double goalFitness = DBL_MAX;
    Hist hist = Hist();
    std::vector<std::vector<double>> inputs;
    std::vector<std::vector<double>> targets;
    std::vector<uint8_t> bestIndividual;

    GAsm();
    explicit GAsm(const std::string& filename);
    // screw C++
    GAsm(const GAsm& other) = delete;
    GAsm& operator=(const GAsm& other) = delete;
    GAsm(GAsm&& other) = delete;
    GAsm& operator=(GAsm&& other) = delete;
    ~GAsm();

//    void makeCheckpoint();
    void save2File(const std::string& filename);

    void evolve(const std::vector<std::vector<double>>& inputs, const std::vector<std::vector<double>>& targets);
    FitnessFunction* fitnessFunction = new Fitness();
    SelectionFunction* selectionFunction = new TournamentSelection(2);
    CrossoverFunction* crossoverFunction = new OnePointCrossover();
    MutationFunction* mutationFunction = new HardMutation();
    GrowFunction* growFunction = new TreeGrow(3);
    static std::vector<uint8_t> normalOps;
    static std::vector<uint8_t> structuralOps;
    static uint8_t END_OP;
};


#endif //GASM_GASM_H
