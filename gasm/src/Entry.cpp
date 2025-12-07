//
// Created by mateu on 16.11.2025.
//

#include "Entry.h"
#include "GAsmParser.h"

Entry::Entry(int generation,
             double bestFitness,
             double avgFitness,
             double avgSize,
             const std::vector<uint8_t>& bestIndividual)
        : generation_(generation),
          bestFitness_(bestFitness),
          avgFitness_(avgFitness),
          avgSize_(avgSize),
          bestIndividual_(bestIndividual
          ) {}

Entry::Entry(nlohmann::json json) {
    generation_   = json.at("generation").get<int>();
    bestFitness_  = json.at("bestFitness").get<double>();
    avgFitness_   = json.at("avgFitness").get<double>();
    avgSize_      = json.at("avgSize").get<double>();

    // Decode best individual
    std::string ascii = json.at("bestIndividual").get<std::string>();
    size_t len = ascii.size();

    uint8_t* bytes = GAsmParser::ascii2Bytecode(ascii, len);
    bestIndividual_.assign(bytes, bytes + len);
    delete[] bytes;
}

nlohmann::json Entry::toJson()
{
    nlohmann::json j;

    j["generation"]   = generation_;
    j["bestFitness"]  = bestFitness_;
    j["avgFitness"]   = avgFitness_;
    j["avgSize"]      = avgSize_;

    // Convert bytecode to ASCII
    if (!bestIndividual_.empty()) {
        std::string ascii = GAsmParser::bytecode2Ascii(
                bestIndividual_.data(),
                bestIndividual_.size()
        );
        j["bestIndividual"] = ascii;
    } else {
        j["bestIndividual"] = "";
    }

    return j;
}

int Entry::getGeneration() const {
    return generation_;
}

void Entry::setGeneration(int generation) {
    generation_ = generation;
}

double Entry::getBestFitness() const {
    return bestFitness_;
}

void Entry::setBestFitness(double bestFitness) {
    bestFitness_ = bestFitness;
}

double Entry::getAvgFitness() const {
    return avgFitness_;
}

void Entry::setAvgFitness(double avgFitness) {
    avgFitness_ = avgFitness;
}

double Entry::getAvgSize() const {
    return avgSize_;
}

void Entry::setAvgSize(double avgSize) {
    avgSize_ = avgSize;
}

const std::vector<uint8_t> &Entry::getBestBytecode() const {
    return bestIndividual_;
}

void Entry::setBestBytecode(const std::vector<uint8_t> &bestBytecode) {
    bestIndividual_ = bestBytecode;
}

Individual Entry::getBestIndividual() const {
    return Individual(bestIndividual_);
}
