//
// Created by mateu on 16.11.2025.
//

#ifndef GASM_ENTRY_H
#define GASM_ENTRY_H


#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "Individual.h"

class Entry {
private:
    int generation_;
    double bestFitness_;
    double avgFitness_;
    double avgSize_;
    std::vector<uint8_t> bestIndividual_;
public:
    // constructors
    Entry(int generation, double bestFitness, double avgFitness, double avgSize, const std::vector<uint8_t>& bestIndividual);
    explicit Entry(nlohmann::json json);

    // methods
    nlohmann::json toJson();

    // getters and setters
    [[nodiscard]] int getGeneration() const;
    void setGeneration(int generation);
    [[nodiscard]] double getBestFitness() const;
    void setBestFitness(double bestFitness);
    [[nodiscard]] double getAvgFitness() const;
    void setAvgFitness(double avgFitness);
    [[nodiscard]] double getAvgSize() const;
    void setAvgSize(double avgSize);
    [[nodiscard]] const std::vector<uint8_t> &getBestBytecode() const;
    void setBestBytecode(const std::vector<uint8_t> &bestBytecode);
    [[nodiscard]] Individual getBestIndividual() const;
};


#endif //GASM_ENTRY_H
