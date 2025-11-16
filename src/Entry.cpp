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
        : _generation(generation),
          _bestFitness(bestFitness),
          _avgFitness(avgFitness),
          _avgSize(avgSize),
          _bestIndividual(bestIndividual
          ) {}

Entry::Entry(nlohmann::json json) {
    _generation   = json.at("generation").get<int>();
    _bestFitness  = json.at("bestFitness").get<double>();
    _avgFitness   = json.at("avgFitness").get<double>();
    _avgSize      = json.at("avgSize").get<double>();

    // Decode best individual
    std::string ascii = json.at("bestIndividual").get<std::string>();
    size_t len = ascii.size();

    uint8_t* bytes = GAsmParser::ascii2Bytecode(ascii, len);
    _bestIndividual.assign(bytes, bytes + len);
    delete[] bytes;
}

nlohmann::json Entry::toJson()
{
    nlohmann::json j;

    j["generation"]   = _generation;
    j["bestFitness"]  = _bestFitness;
    j["avgFitness"]   = _avgFitness;
    j["avgSize"]      = _avgSize;

    // Convert bytecode to ASCII
    if (!_bestIndividual.empty()) {
        std::string ascii = GAsmParser::bytecode2Ascii(
                _bestIndividual.data(),
                _bestIndividual.size()
        );
        j["bestIndividual"] = ascii;
    } else {
        j["bestIndividual"] = "";
    }

    return j;
}
