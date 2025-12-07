//
// Created by mateu on 16.11.2025.
//

#ifndef GASM_HIST_H
#define GASM_HIST_H


#include <vector>
#include "Entry.h"

class Hist {
private:
    std::vector<Entry> entries_;
public:
    // constructors
    Hist();
    explicit Hist(nlohmann::json json);

    // methods
    void add(int generation, double bestFitness, double avgFitness, double avgSize, const std::vector<uint8_t>& bestIndividual);
    nlohmann::json toJson();

    // getters and setters
    [[nodiscard]] const std::vector<Entry>& getEntries() const;
    [[nodiscard]] const Entry& getEntry(size_t i) const;
    [[nodiscard]] const Entry& getLast() const;
};


#endif //GASM_HIST_H
