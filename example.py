import gasm

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
g.maxGenerations = 3
g.goalFitness = 0.0
g.minimize = True
g.registerLength = 10
g.maxProcessTime = 10000
g.outputFolder = "data/checkpoints"
g.nanPenalty = 1e1
g.useCompile = True
g.checkpointInterval = 2
g.setSelection("Tournament", 3) # Literal["Tournament", "Truncation", "Boltzman", "Rank", "Roulette"], number works set temperature or tournament size
g.setGrow("Size", 10)           # Literal["Size", "Full", "Tree"], number works set depth or start size
g.setMutation("Hard")           # Literal["Hard", "Soft"]
g.setCrossover("TwoPointSize")  # Literal["OnePoint", "TwoPoint", "TwoPointSize", "Uniform"]
# g.set_cng("increment", 1)       #  Literal["increment", "constant"], number does not work XD, it crashes don't use XD
# g.set_rng("random", 1)          #  Literal["random", "constant"], number does not work XD, it crashes don't use XD

inputs = [[1], [2], [3], [4], [5], [6], [7], [8], [9], [10],]

targets = [[1], [1], [2], [3], [5], [8], [13], [21], [34], [55],]

g.evolve(inputs, targets)

g.save2File("data/example.json")

# Continue from file DON'T CHANGE POPULATION SIZE
g = gasm.GAsm.fromJson("data/example.json")
g.maxGenerations = 7
g.evolve(inputs, targets)

hist = g.hist
best_individual = g.best

print("length: ", len(hist))

entry = hist[0]
entries = [e for e in hist]
print(f"generation: {entry.generation}")
print(f"best_fitness: {entry.best_fitness}")
print(f"avg_fitness: {entry.avg_fitness}")
print(f"avg_size: {entry.avg_size}")
print(entry.best.toString())
print(entries)

input = [1.0]
time = best_individual.run(input)
print(f"Process time: {time}")
print(f"Result: {input}")

print(best_individual.toString())

best_individual.set_cng("increment", 1)
best_individual.set_rng("random", 1)
best_individual.maxProcessTime = 10000
best_individual.registerLength = 10
best_individual.compile = True
print("===== END =====")