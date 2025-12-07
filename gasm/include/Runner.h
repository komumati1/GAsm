//
// Created by mateu on 07.12.2025.
//

#ifndef GASM_RUNNER_H
#define GASM_RUNNER_H

#include "GAsmInterpreter.h"
#include <vector>


class Runner {
private:
    GAsmInterpreter jit_ = GAsmInterpreter(1);
    std::unique_ptr<FitnessFunction> fitnessFunction_ = std::make_unique<Fitness>();
    std::unique_ptr<SelectionFunction> selectionFunction_ = std::make_unique<TournamentSelection>(2);
    std::unique_ptr<CrossoverFunction> crossoverFunction_ = std::make_unique<OnePointCrossover>();
    std::unique_ptr<MutationFunction> mutationFunction_ = std::make_unique<HardMutation>();
    std::unique_ptr<GrowFunction> growFunction_ = std::make_unique<FullGrow>();
public:
    friend class GAsm;
    // constructors
    Runner() = default;
    Runner(const Runner& other);
    Runner& operator=(const Runner& other);
    Runner(Runner&& other) noexcept;
    Runner& operator=(Runner&& other) noexcept;
    ~Runner() = default;

    // setters and getters
    [[nodiscard]] const FitnessFunction& fitness() const { return *fitnessFunction_; }
    void setFitnessFunction(std::unique_ptr<FitnessFunction> f) { fitnessFunction_ = std::move(f); }
    [[nodiscard]] const SelectionFunction& selection() const { return *selectionFunction_; }
    void setSelectionFunction(std::unique_ptr<SelectionFunction> s) { selectionFunction_ = std::move(s); }
    [[nodiscard]] const CrossoverFunction& crossover() const { return *crossoverFunction_; }
    void setCrossoverFunction(std::unique_ptr<CrossoverFunction> c) { crossoverFunction_ = std::move(c); }
    [[nodiscard]] const MutationFunction& mutation() const { return *mutationFunction_; }
    void setMutationFunction(std::unique_ptr<MutationFunction> m) { mutationFunction_ = std::move(m); }
    [[nodiscard]] const GrowFunction& grow() const { return *growFunction_; }
    void setGrowFunction(std::unique_ptr<GrowFunction> g) { growFunction_ = std::move(g); }

    // jit setters and getters
    [[nodiscard]] size_t getRegisterLength() const { return jit_.getRegisterLength(); }
    void setRegisterLength(const size_t& length) { jit_.setRegisterLength(length); }
    [[nodiscard]] const gen_fn_t& getCNG() const { return jit_.getCng(); }
    void setCNG(std::unique_ptr<gen_fn_t> cng) { jit_.setCng(std::move(cng)); }
    [[nodiscard]] const gen_fn_t& getRNG() const { return jit_.getRng(); }
    void setRNG(std::unique_ptr<gen_fn_t> rng) { jit_.setRng(std::move(rng)); }
    [[nodiscard]] const bool& getCompile() const { return jit_.useCompile; }
    void setCompile(const bool& useCompile) { jit_.useCompile = useCompile; }

    // methods
    void dispatchGrow(GAsm* gasm, size_t start, size_t end, bool verbose);
    void dispatchEvolve(GAsm* gasm, size_t start, size_t end, bool verbose);
};

#endif //GASM_RUNNER_H
