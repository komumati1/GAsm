//
// Created by mateu on 06.12.2025.
//

#ifndef GASM_INDIVIDUAL_H
#define GASM_INDIVIDUAL_H

#include "GAsmInterpreter.h"
#include "GAsmParser.h"
#include <utility>
#include <vector>

class Individual {
private:
    GAsmInterpreter jit_;
    std::vector<uint8_t> bytecode_;
public:
    // constructors
    explicit Individual(std::vector<uint8_t> bytecode) : jit_(bytecode, 1) , bytecode_(std::move(bytecode)){ jit_.setProgram(bytecode_); }
    explicit Individual(const std::string& text) : jit_(1) {
        size_t len;
        uint8_t* bytes = GAsmParser::text2Bytecode(text, len);
        bytecode_.assign(bytes, bytes + len);
        jit_.setProgram(bytecode_);
    }
    Individual(const Individual& other) : jit_(other.jit_), bytecode_(other.bytecode_) {}
    Individual& operator=(const Individual& other) {
        if (this != &other) {
            jit_ = other.jit_;
            bytecode_ = other.bytecode_;
        }
        return *this;}
    Individual(Individual&& other) noexcept : jit_(std::move(other.jit_)), bytecode_(std::move(other.bytecode_)) {}
    Individual& operator=(Individual&& other) noexcept {
        if (this != &other) {
            jit_ = std::move(other.jit_);
            bytecode_ = std::move(other.bytecode_);
        }
        return *this;}
    ~Individual() = default;

    // runner setters and getters
    [[nodiscard]] size_t getRegisterLength() const { return jit_.getRegisterLength(); }
    void setRegisterLength(const size_t& length) { jit_.setRegisterLength(length); }
    [[nodiscard]] const gen_fn_t& getCNG() const { return jit_.getCng(); }
    void setCNG(std::unique_ptr<gen_fn_t> cng) { jit_.setCng(std::move(cng)); }
    [[nodiscard]] const gen_fn_t& getRNG() const { return jit_.getRng(); }
    void setRNG(std::unique_ptr<gen_fn_t> rng) { jit_.setRng(std::move(rng)); }
    [[nodiscard]] const bool& getCompile() const { return jit_.useCompile; }
    void setCompile(const bool& useCompile) { jit_.useCompile = useCompile; }
    [[nodiscard]] const std::vector<uint8_t>& getBytecode() const { return bytecode_; }

    // public runner attributes
    size_t maxProcessTime = 10000;

    // methods
    size_t run(std::vector<double>& inputs) { return jit_.run(inputs, maxProcessTime); };
    std::string toString() { return GAsmParser::bytecode2Text(bytecode_.data(), bytecode_.size()); }
};


#endif //GASM_INDIVIDUAL_H
