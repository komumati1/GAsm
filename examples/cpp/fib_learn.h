
#include "GAsm.h"

void fibEvolution() {
    auto gasm = GAsm();
    gasm.setRegisterLength(10);
    // global
    gasm.maxGenerations = 100;
    gasm.populationSize = 100000;
    gasm.individualMaxSize = 100;
    gasm.minimize = true;
    gasm.goalFitness = 0.0;
    gasm.maxProcessTime = 10000;
    gasm.mutationProbability = 0.1;
    // functions
//    gasm.fitnessFunction = new FibFitness();
//    gasm.crossoverFunction = new UniformPointCrossover();
//    gasm.selectionFunction = new TournamentSelection(3);
//    gasm.selectionFunction = new BoltzmannSelection(3);
//    gasm.growFunction = new TreeGrow(3);
//    gasm.mutationFunction = new HardMutation();
//    gasm.setGrowFunction(std::make_unique<TreeGrow>(3));

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
    gasm.parallelEvolve(inputs, targets);
    // save
    gasm.save2File("../../../data/fib3.json");
}