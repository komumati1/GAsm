//
// Created by mateu on 16.11.2025.
//

#include "Hist.h"

Hist::Hist() : _entries() {}

Hist::Hist(nlohmann::json json)
{
    if (!json.contains("entries"))
        return;

    for (auto& e : json["entries"])
        _entries.emplace_back(e);
}

void Hist::add(int generation,
               double bestFitness,
               double avgFitness,
               double avgSize,
               const std::vector<uint8_t>& bestIndividual)
{
    _entries.emplace_back(
            generation, bestFitness, avgFitness, avgSize, bestIndividual
    );
}

nlohmann::json Hist::toJson()
{
    nlohmann::json j;
    j["entries"] = nlohmann::json::array();

    for (Entry entry : _entries)
        j["entries"].push_back(entry.toJson());

    return j;
}
