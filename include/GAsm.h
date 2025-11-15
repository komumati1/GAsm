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

class GAsm {
private:
    std::vector<std::vector<uint8_t>> _population;
    std::vector<double> _fitness;
    std::vector<double> _rank;  // other parameter determining the quality of individual
public:
    unsigned int populationSize = 100000;
    unsigned int individualMaxSize = 10000;
    double mutationProbability = 0.05;
    double crossoverProbability = 0.9;
    unsigned int tournamentSize = 2;
    unsigned int maxGenerations = 100;
    std::vector<std::vector<double>> inputs;
    std::vector<std::vector<double>> targets;

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
    std::function<void(GAsm* self, std::vector<uint8_t>& individual)> growFunction = fullGrow;
    std::function<double(GAsm* self, const std::vector<uint8_t>& individual)> fitnessFunction = fitness;
    std::function<size_t(GAsm* self)> selectionFunction = tournamentSelection;
    std::function<size_t(GAsm* self)> negativeSelectionFunction = negativeTournamentSelection;
    std::function<void(GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual1, const std::vector<uint8_t>& bestIndividual2)> crossoverFunction = dumbCrossover;
    std::function<void(GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual)> mutationFunction = dumbMutation;

    static void fullGrow(GAsm* self, std::vector<uint8_t>& individual);
    static double fitness(GAsm* self, const std::vector<uint8_t>& individual);
    static size_t tournamentSelection(GAsm* self);
    static size_t negativeTournamentSelection(GAsm* self);
    static void dumbCrossover(GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual1, const std::vector<uint8_t>& bestIndividual2);
    static void dumbMutation(GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual);
};


#endif //GASM_GASM_H
