//
// Created by mateu on 16.11.2025.
//

#include "Hist.h"

Hist::Hist() : entries_() {}

Hist::Hist(nlohmann::json json)
{
    if (!json.contains("entries"))
        return;

    for (auto& e : json["entries"])
        entries_.emplace_back(e);
}

void Hist::add(int generation,
               double bestFitness,
               double avgFitness,
               double avgSize,
               const std::vector<uint8_t>& bestIndividual)
{
    entries_.emplace_back(
            generation, bestFitness, avgFitness, avgSize, bestIndividual
    );
}

nlohmann::json Hist::toJson()
{
    nlohmann::json j;
    j["entries"] = nlohmann::json::array();

    for (Entry entry : entries_)
        j["entries"].push_back(entry.toJson());

    return j;
}

const std::vector<Entry> &Hist::getEntries() const {
    return entries_;
}

const Entry &Hist::getEntry(size_t i) const {
    return entries_[i];
}

const Entry &Hist::getLast() const {
    return entries_.back();
}
