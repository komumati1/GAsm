//
// Created by mateu on 07.12.2025.
//

#include "Runner.h"
#include "utils.h"
#include "GAsm.h"

Runner::Runner(const Runner &other)
    : jit_(other.jit_),
      fitnessFunction_(other.fitnessFunction_->clone()),
      selectionFunction_(other.selectionFunction_->clone()),
      crossoverFunction_(other.crossoverFunction_->clone()),
      mutationFunction_(other.mutationFunction_->clone()),
      growFunction_(other.growFunction_->clone()) {

}

Runner &Runner::operator=(const Runner &other) {
    if (this != &other) {
        jit_ = other.jit_;
        fitnessFunction_ = other.fitnessFunction_->clone();
        selectionFunction_ = other.selectionFunction_->clone();
        crossoverFunction_ = other.crossoverFunction_->clone();
        mutationFunction_ = other.mutationFunction_->clone();
        growFunction_ = other.growFunction_->clone();
    }
    return *this;
}

Runner::Runner(Runner &&other) noexcept {
    jit_ = std::move(other.jit_);
    fitnessFunction_ = std::move(other.fitnessFunction_);
    selectionFunction_ = std::move(other.selectionFunction_);
    crossoverFunction_ = std::move(other.crossoverFunction_);
    mutationFunction_ = std::move(other.mutationFunction_);
    growFunction_ = std::move(other.growFunction_);
}

Runner &Runner::operator=(Runner &&other) noexcept {
    if (this != &other) {
        jit_ = std::move(other.jit_);
        fitnessFunction_ = std::move(other.fitnessFunction_);
        selectionFunction_ = std::move(other.selectionFunction_);
        crossoverFunction_ = std::move(other.crossoverFunction_);
        mutationFunction_ = std::move(other.mutationFunction_);
        growFunction_ = std::move(other.growFunction_);
    }
    return *this;
}

void Runner::dispatchGrow(GAsm *gasm, size_t start, size_t end, bool verbose) {
    size_t size = end - start;
    auto initStart = std::chrono::high_resolution_clock::now();
    for (size_t i = start; i < end; i++) {
        if (verbose) {
            int progress = ((int) i + 1) * 100 / (int) size;
            double elapsed = std::chrono::duration<double>(
                    std::chrono::high_resolution_clock::now() - initStart).count();
            printProgressBar(progress, elapsed);
        }
        (*growFunction_)(gasm, gasm->population_[i]);
        std::pair<double, double> fitRank = (*fitnessFunction_)(gasm, jit_, gasm->population_[i]);
        gasm->fitness_[i] = fitRank.first;
        gasm->rank_[i] = fitRank.second;
    }
}

void Runner::dispatchEvolve(GAsm *gasm, size_t start, size_t end, bool verbose) {
    size_t size = end - start;
    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<double> dist(0, 1);
    auto genStart = std::chrono::high_resolution_clock::now();
    for (size_t i = start; i < end; i++) {
        selectionFunction_->selectMinimal = !gasm->minimize; // worst is not minimized
        size_t worstIndex = (*selectionFunction_)(gasm);
        std::vector<uint8_t> worstInd = gasm->getIndividual(worstIndex);
        selectionFunction_->selectMinimal = gasm->minimize;  // best is minimized
        if (dist(engine) < gasm->crossoverProbability) {
            size_t bestIndex1 = (*selectionFunction_)(gasm);
            size_t bestIndex2 = (*selectionFunction_)(gasm);
            std::vector<uint8_t> bestInd1 = gasm->getIndividual(bestIndex1);
            std::vector<uint8_t> bestInd2 = gasm->getIndividual(bestIndex2);

            (*crossoverFunction_)(gasm, worstInd, bestInd1, bestInd2);
        } else {
            size_t bestIndex = (*selectionFunction_)(gasm);
            std::vector<uint8_t> bestInd = gasm->getIndividual(bestIndex);
            (*mutationFunction_)(gasm, worstInd, bestInd);
        }

        std::pair<double, double> fitRank = (*fitnessFunction_)(gasm, jit_, worstInd);
        gasm->setIndividual(worstIndex, worstInd, fitRank.first, fitRank.second);
        if (verbose) {
            int progress = ((int)i + 1) * 100 / (int) size;
            double elapsed = std::chrono::duration<double>(
                    std::chrono::high_resolution_clock::now() - genStart).count();
            printProgressBar(progress, elapsed);
        }
    }
}