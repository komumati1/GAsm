//
// Created by mateu on 16.11.2025.
//

#ifndef GASM_ENTRY_H
#define GASM_ENTRY_H


#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

class Entry {
private:
    int _generation;
    double _bestFitness;
    double _avgFitness;
    double _avgSize;
    std::vector<uint8_t> _bestIndividual;
public:
    Entry(int generation, double bestFitness, double avgFitness, double avgSize, const std::vector<uint8_t>& bestIndividual);
    explicit Entry(nlohmann::json json);

    nlohmann::json toJson();
};


#endif //GASM_ENTRY_H
