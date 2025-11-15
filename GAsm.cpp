//
// Created by mateu on 14.11.2025.
//

#include "GAsm.h"

GAsm::GAsm(unsigned int population_size,
           unsigned int individual_max_size,
           double permutation_probability,
           double crossover_probability,
           unsigned int tournament_size,
           unsigned int max_generations) :
           _population_size(population_size),
           _individual_max_size(individual_max_size),
           _permutation_probability(permutation_probability),
           _crossover_probability(crossover_probability),
           _tournament_size(tournament_size),
           _max_generations(max_generations) {

}
