import gasm
from data_generators import gen_11_anypos_constant, gen_binop_sum_sub_mul, gen_max_dataset, gen_mean10_dataset, \
    gen_min4_dataset, gen_neg_to_zero_dataset, make_random_truth_table, gen_boolean_dataset, gen_arith_sequence_dataset

"""
1. Change the CMakeLists.txt, line 106, change to your interpreter (3.11)
2. run python setup.py build_ext --inplace
3. (Optional) pip install gasm
Check gasm/python/gasm.pyi and gasm/python/gasm.py for documentation
"""

g = gasm.GAsm()
g.populationSize = 10000
g.individualMaxSize = 50
g.mutationProbability = 0.2
g.crossoverProbability = 0.9
g.maxGenerations = 100
g.goalFitness = 0.0
g.minimize = True
g.registerLength = 10
g.maxProcessTime = 10000
g.outputFolder = "data/checkpoints"
g.nanPenalty = 1e3
g.useCompile = True
g.checkpointInterval = 2
g.setSelection("Tournament", 1) # Literal["Tournament", "Truncation", "Boltzman", "Rank", "Roulette"], number works set temperature or tournament size
g.setGrow("Size", 12)           # Literal["Size", "Full", "Tree"], number works set depth or start size
g.setMutation("Hard")           # Literal["Hard", "Soft"]
g.setCrossover("TwoPointSize")  # Literal["OnePoint", "TwoPoint", "TwoPointSize", "Uniform"]
# g.set_cng("increment", 1)       #  Literal["increment", "constant"], number does not work XD, it crashes don't use XD
# g.set_rng("random", 1)          #  Literal["random", "constant"], number does not work XD, it crashes don't use XD


def start_learning(test_case: str, length):
    g = gasm.GAsm()
    g.populationSize = 10000
    g.individualMaxSize = 50
    g.mutationProbability = 0.35
    g.crossoverProbability = 0.9
    g.maxGenerations = 100
    g.goalFitness = 0.0
    g.minimize = True
    g.registerLength = length
    g.maxProcessTime = 10000
    g.outputFolder = "data/checkpoints"
    g.nanPenalty = 1e2
    g.useCompile = True
    g.checkpointInterval = 10
    g.setSelection("Tournament", 1) # Literal["Tournament", "Truncation", "Boltzman", "Rank", "Roulette"], number works set temperature or tournament size
    g.setGrow("Size", 12)           # Literal["Size", "Full", "Tree"], number works set depth or start size
    g.setMutation("Hard")           # Literal["Hard", "Soft"]
    g.setCrossover("TwoPointSize")  # Literal["OnePoint", "TwoPoint", "TwoPointSize", "Uniform"]
    # g.set_cng("increment", 1)       #  Literal["increment", "constant"], number does not work XD, it crashes don't use XD
    # g.set_rng("random", 1)          #  Literal["random", "constant"], number does not work XD, it crashes don't use XD


    g.evolve(inputs, targets)

    g.save2File(f"data/example_{test_case}.json")


test_case = "final_radom_mul_1"

# 1.1.B
# inputs, targets = gen_11_anypos_constant(789, n=100, buf=100, seed=42)
inputs, targets = gen_binop_sum_sub_mul("mul", n_random=100, buf=10, seed=1)  # 1.2.C

# inputs, targets = gen_max_dataset(lo=-50, hi=50, n=800, buf=10, seed=42)
# inputs, targets = gen_mean10_dataset(lo=-50, hi=50, n=800, buf=10, seed=7)

# inputs, targets = gen_min4_dataset(lo=-9, hi=9, n=800, buf=10, seed=1)

# inputs, targets = gen_neg_to_zero_dataset(lo=-9, hi=9, n=800, buf=10, seed=1)

# for k in range(5, 11):
#     f_rand, _ = make_random_truth_table(k, seed=100+k)
#     inputs, targets = gen_boolean_dataset(k, f_rand, buf=max(12, k+2), add_constants=True)
#
#     # print(inputs[0])
#     # print(targets[0])
#     start_learning(f"{test_case}_0{k}", max(12, k+2))
#     print(f"===== END for k={k} =====")


#
# Tmax =5
# buf = Tmax + 5

# inputs, targets = gen_arith_sequence_dataset(n=100, Tmax=Tmax, buf=buf, seed=42)
# print(inputs[12])
# print(targets[12])
# start_learning(test_case, buf)




# print(inputs[1])
# print(targets[1])

g.evolve(inputs, targets)
#
g.save2File(f"data/example_{test_case}.json")
#
# # Continue from file DON'T CHANGE POPULATION SIZE
# # g = gasm.GAsm.fromJson(f"data/example_{test_case}.json")
# # g.maxGenerations = 7
# # g.evolve(inputs, targets)
#
# hist = g.hist
# best_individual = g.best
#
# print("length: ", len(hist))
#
# entry = hist[0]
# entries = [e for e in hist]
# print(f"generation: {entry.generation}")
# print(f"best_fitness: {entry.best_fitness}")
# print(f"avg_fitness: {entry.avg_fitness}")
# print(f"avg_size: {entry.avg_size}")
# # print(entry.best.toString())
# # print(entries)
#
# # input = [1.0, 2.0, 3.0]
# input = [float(x) for x in inputs[0]]
# time = best_individual.run(input)
# print(f"Process time: {time}")
# print(f"Result: {input}")
#
# # print(best_individual.toString())
#
# best_individual.set_cng("increment", 1)
# best_individual.set_rng("random", 1)
# best_individual.maxProcessTime = 10000
# best_individual.registerLength = 10
# best_individual.compile = True
# print("===== END =====")