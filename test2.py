import gasm

g = gasm.GAsm()

g.registerLength = 10
g.maxGenerations = 5
g.populationSize = 10000
g.individualMaxSize = 50
g.minimize = True
g.goalFitness = 0.0
g.maxProcessTime = 10000
g.mutationProbability = 0.2

inputs = [[1], [2], [3], [4], [5], [6], [7], [8], [9], [10],]

targets = [[1], [1], [2], [3], [5], [8], [13], [21], [34], [55],]

g.evolve(inputs, targets)
g.save2File("test.json")