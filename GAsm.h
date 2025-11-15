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

class GAsm {
private:
    uint8_t** _population;
    size_t* _individual_lengths;
    double* _fitness;
    double* _rank;  // other parameter determining the quality of individual
    size_t _population_length;
    unsigned int _population_size;
    unsigned int _individual_max_size;
    double _permutation_probability;
    double _crossover_probability;
    unsigned int _tournament_size;
    unsigned int _max_generations;
public:
    GAsm(unsigned int population_size,
         unsigned int individual_max_size,
         double permutation_probability,
         double crossover_probability,
         unsigned int tournament_size,
         unsigned int max_generations);
};


#endif //GASM_GASM_H
