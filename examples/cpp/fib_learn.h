//
// Created by mateu on 26.11.2025.
//

#include "GAsm.h"

class FibFitness : public FitnessFunction {
public:
    FibFitness() = default;
    std::pair<double, double> operator()(GAsm* self, const std::vector<uint8_t>& individual) override {
        self->setProgram(const_cast<std::vector<uint8_t> &>(individual));
        double score = 0.0;
        double avgTime = 0.0;
        for (int i = 0; i < self->inputs.size(); i++) {
            std::vector<double> input = self->inputs[i];
            const std::vector<double>& target = self->targets[i];
            avgTime += (double)self->run(input);
            score += fabs(input[0] - target[0]); // TODO handle NaN
        }
        avgTime /= (double)self->inputs.size();
        return {score, avgTime};
    }
};

void fibEvolution() {
    auto gasm = GAsm();
    gasm.setRegisterLength(10);
    // global
    gasm.maxGenerations = 100;
    gasm.populationSize = 10000;
    gasm.individualMaxSize = 100;
    gasm.minimize = true;
    gasm.goalFitness = 0.0;
    gasm.maxProcessTime = 10000;
    gasm.mutationProbability = 0.2;
    // functions
//    gasm.fitnessFunction = new FibFitness();
//    gasm.crossoverFunction = new UniformPointCrossover();
//    gasm.selectionFunction = new TournamentSelection(3);
//    gasm.growFunction = new FullGrow();
//    gasm.mutationFunction = new HardMutation();

    std::vector<uint8_t> program = {
            MOV_R_A,
            INC,
            MOV_A_P,
            MOV_R_A,
            LOP_A,
            DEC,
            MOV_A_R,
            INC,
            ADD_R,
            INC,
            MOV_R_A,
            MOV_A_P,
            END,
            MOV_A_R,
            MOV_I_A
    };

    // input and target vectors
    std::vector<std::vector<double>> inputs = {
            {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}, {10},
//            {11}, {12}, {13}, {14}, {15}, {16}, {17}, {18}, {19}, {20}
    };

    std::vector<std::vector<double>> targets = {
            {1}, {1}, {2}, {3}, {5}, {8}, {13}, {21}, {34}, {55},
//            {89}, {144}, {233}, {377}, {610}, {987}, {1597}, {2584}, {4181}, {6765}
    };
    // evolution
    gasm.evolve(inputs, targets);
    // save
    gasm.save2File("../data/fib2.json");
}