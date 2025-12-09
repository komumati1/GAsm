
#include "GAsm.h"
#include "GAsmInterpreter.h"

void fibEvolution() {
//    auto gasm = GAsm("../../../data/checkpoints/2025-12-08_15-02-54.json");
//    gasm.maxGenerations = 20;
    auto gasm = GAsm();
    // global
    gasm.setRegisterLength(10);
    gasm.maxGenerations = 5;
    gasm.populationSize = 1000;
    gasm.individualMaxSize = 100;
    gasm.minimize = true;
    gasm.goalFitness = 0.0;
    gasm.maxProcessTime = 10000;
    gasm.mutationProbability = 0.1;
    gasm.outputFolder = "../../../data/checkpoints";
    gasm.checkPointInterval = 20;
    // functions
//    gasm.fitnessFunction = new FibFitness();
//    gasm.crossoverFunction = new UniformPointCrossover();
//    gasm.selectionFunction = new TournamentSelection(3);
//    gasm.selectionFunction = new BoltzmannSelection(3);
//    gasm.growFunction = new TreeGrow(3);
//    gasm.mutationFunction = new HardMutation();
    gasm.setGrowFunction(std::make_unique<SizeGrow>(10));
    gasm.setCrossoverFunction(std::make_unique<TwoPointSizeCrossover>());
    gen_fn_t func = [](){return 0.0;};
    auto fn_ptr = std::make_unique<gen_fn_t >(func);
    gasm.setCNG(std::make_unique<gen_fn_t>(*fn_ptr));

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