//
// Created by mateu on 20.11.2025.
//

#ifndef GASM_GASMINTERPRETER_H
#define GASM_GASMINTERPRETER_H

#include <functional>
#include "xbyak.h"
#include "functions.h"

// function type returned by compile(...)
using run_fn_t = size_t (*)(double* inputs, size_t inputLength,
                            double* registers, size_t registerLength,
                            double (*constants)(),
                            double (*rng)(),
                            size_t maxProcessTime);

using gen_fn_t = double(*)();

class GAsmInterpreter {
private:
    const std::vector<uint8_t>* program_;
    std::vector<double> registers_;
    Xbyak::CodeGenerator code_;
    run_fn_t compiled_;

    std::unique_ptr<gen_fn_t> cng_ = std::make_unique<gen_fn_t>([](){
                static size_t counter = 0;
                return (double) counter++;});
    std::unique_ptr<gen_fn_t> rng_ = std::make_unique<gen_fn_t>([](){
                static thread_local std::mt19937 engine(std::random_device{}());
                static std::uniform_real_distribution<double> dist(0, 1);
                return dist(engine);});
public:
    // getters and setters
    [[nodiscard]] run_fn_t compile();
    void setProgram(const std::vector<uint8_t>& program);
    [[nodiscard]] size_t getRegisterLength() const { return registers_.size(); }
    void setRegisterLength(size_t registerLength);
    [[nodiscard]] const gen_fn_t& getCng() const { return *cng_; }
    void setCng(std::unique_ptr<gen_fn_t> cng) { cng_ = std::move(cng); }
    [[nodiscard]] const gen_fn_t& getRng() const { return *rng_; }
    void setRng(std::unique_ptr<gen_fn_t> rng) { rng_ = std::move(rng); }

    // public attributes
    bool useCompile = true;

    // constructors
    explicit GAsmInterpreter(const std::vector<uint8_t>& program, size_t registerLength);
    explicit GAsmInterpreter(size_t registerLength);
    GAsmInterpreter(const GAsmInterpreter& other);
    GAsmInterpreter& operator=(const GAsmInterpreter& other);
    GAsmInterpreter(GAsmInterpreter&& other) noexcept;
    GAsmInterpreter& operator=(GAsmInterpreter&& other) noexcept;
    ~GAsmInterpreter() = default;

    // methods
    size_t run(std::vector<double> &inputs, size_t maxProcessTime);
    size_t runInterpreter(std::vector<double> &inputs, size_t maxProcessTime);
    size_t runCompiled(std::vector<double> &inputs, size_t maxProcessTime);
};


#endif //GASM_GASMINTERPRETER_H
