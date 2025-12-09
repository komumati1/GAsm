//
// Created by mateu on 08.12.2025.
//

#include "utils.h"

std::unique_ptr<gen_fn_t> makeCNG(const std::string& spec, double start) {
    if (spec == "increment") {
        return std::make_unique<gen_fn_t>([](){
            static thread_local size_t counter = 0;
            return (double) counter++;});
    }

    if (spec == "constant") {
        return std::make_unique<gen_fn_t>([](){
            return 1.0;});
    }

    throw std::runtime_error("Invalid CNG literal");
}

std::unique_ptr<gen_fn_t> makeRNG(const std::string& spec, double start) {
    if (spec == "random") {
        return std::make_unique<gen_fn_t>([](){
            static thread_local std::mt19937 engine(std::random_device{}());
            static std::uniform_real_distribution<double> dist(0, 1);
            return dist(engine);});
    }

    if (spec == "constant") {
        return std::make_unique<gen_fn_t>([](){
            return 1.0;});
    }

    throw std::runtime_error("Invalid RNG literal");
}