//
// Created by mateu on 16.11.2025.
//

#ifndef GASM_HIST_H
#define GASM_HIST_H


#include <vector>
#include "Entry.h"

class Hist {
private:
    std::vector<Entry> _entries;
public:
    Hist();
    explicit Hist(nlohmann::json json);

    void add(int generation, double bestFitness, double avgFitness, double avgSize, const std::vector<uint8_t>& bestIndividual);
    nlohmann::json toJson();
};


#endif //GASM_HIST_H
