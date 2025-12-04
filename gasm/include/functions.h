//
// Created by mateu on 20.11.2025.
//

#ifndef GASM_FUNCTIONS_H
#define GASM_FUNCTIONS_H


#include <vector>
#include <cstdint>
#include <chrono>
#include <thread>
#include <random>

class GAsm;

class FitnessFunction {
public:
    virtual ~FitnessFunction() = default;
    virtual std::pair<double, double> operator()(GAsm* self, const std::vector<uint8_t>& individual) = 0;
};

class Fitness : public FitnessFunction {
public:
    Fitness() = default;
    std::pair<double, double> operator()(GAsm* self, const std::vector<uint8_t>& individual) override;
};

class SelectionFunction {
public:
    bool selectMinimal = true;
    virtual ~SelectionFunction() = default;
    virtual size_t operator()(const GAsm* self) = 0;
};

class TournamentSelection : public SelectionFunction {
private:
    size_t _tournamentSize;
public:
    explicit TournamentSelection(size_t tournamentSize) noexcept : _tournamentSize(tournamentSize) {};
    size_t operator()(const GAsm* self) override;
};

class RouletteSelection : public SelectionFunction {
public:
    explicit RouletteSelection() {}
    size_t operator()(const GAsm* self) override;
};

class RankSelection : public SelectionFunction {
public:
    explicit RankSelection() {}
    size_t operator()(const GAsm* self) override;
};

class TruncationSelection : public SelectionFunction {
private:
    double _percent;  // e.g. 0.1 = top 10%
public:
    explicit TruncationSelection(double percent) noexcept : _percent(percent) {}
    size_t operator()(const GAsm* self) override;
};

class BoltzmannSelection : public SelectionFunction {
private:
    double _temperature;
public:
    explicit BoltzmannSelection(double T) noexcept : _temperature(T) {}
    size_t operator()(const GAsm* self) override;
};



class CrossoverFunction {
public:
    virtual ~CrossoverFunction() = default;
    virtual void operator()(const GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual1, const std::vector<uint8_t>& bestIndividual2) = 0;
};

class OnePointCrossover : public CrossoverFunction {
public:
    OnePointCrossover() = default;
    void operator()(const GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual1, const std::vector<uint8_t>& bestIndividual2) override;
};

class TwoPointCrossover : public CrossoverFunction {
public:
    TwoPointCrossover() = default;
    void operator()(const GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual1, const std::vector<uint8_t>& bestIndividual2) override;
};

class UniformPointCrossover : public CrossoverFunction {
public:
    UniformPointCrossover() = default;
    void operator()(const GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual1, const std::vector<uint8_t>& bestIndividual2) override;
};

class MutationFunction {
public:
    virtual ~MutationFunction() = default;
    virtual void operator()(const GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual) = 0;
};

class HardMutation : public MutationFunction {
public:
    HardMutation() = default;
    void operator()(const GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual) override;
};

class SoftMutation : public MutationFunction {
public:
    SoftMutation() = default;
    void operator()(const GAsm* self, std::vector<uint8_t>& worstIndividual, const std::vector<uint8_t>& bestIndividual) override;
};

class GrowFunction {
public:
    virtual ~GrowFunction() = default;
    virtual void operator()(const GAsm* self, std::vector<uint8_t>& individual) = 0;
};

class FullGrow : public GrowFunction {
public:
    FullGrow() = default;
    void operator()(const GAsm* self, std::vector<uint8_t>& individual) override;
};

class TreeGrow : public GrowFunction {
private:
    size_t _depth;
    void grow(std::vector<uint8_t>& individual, size_t maxSize);
public:
    explicit TreeGrow(size_t depth) : _depth(depth) {};
    void operator()(const GAsm* self, std::vector<uint8_t>& individual) override;
};


#endif //GASM_FUNCTIONS_H
