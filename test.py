import gasm
from data_generators import gen_11_anypos_constant, gen_binop_sum_sub_mul

# Continue from file DON'T CHANGE POPULATION SIZE

test_case = "1_2_C_1"
g = gasm.GAsm.fromJson(f"data/example_{test_case}.json")

hist = g.hist
best_individual = g.best

# print("length: ", len(hist))

entry = hist[0]
entries = [e for e in hist]
# print(f"generation: {entry.generation}")
# print(f"best_fitness: {entry.best_fitness}")
# print(f"avg_fitness: {entry.avg_fitness}")
# print(f"avg_size: {entry.avg_size}")
# print(entry.best.toString())
# print(entries)

# inputs, targets = gen_11_anypos_constant(789, n=100, buf=100, seed=42)

inputs, targets = gen_binop_sum_sub_mul("mul", n_random=500, buf=10, seed=1)  # 1.2.C

num = 0

def test_model(test_case, num1, num2):
    SENT = 1234567.89
    g = gasm.GAsm.fromJson(f"data/example_{test_case}.json")
    hist = g.hist
    best_individual = g.best

    inputs = [SENT for _ in range(10)]

    inputs[0] = num1
    inputs[1] = num2

    time = best_individual.run(inputs)
    print(f"Result: {[int(x) for x in inputs]}")
    print(best_individual.toString())


input = [float(x) for x in inputs[num]]
input[0] = 37.0
input[1] = 3333.0

print(input)
print(targets[num])
# print(inputs[0])
# input = [float(x) for x in range(100)]
# import random
# rng = random.Random(1)
# input = [float(rng.randint(-100, 100)) for _ in range(100)]
time = best_individual.run(input)
# print(f"Process time: {time}")
print(f"Result: {[int(x) for x in input]}")
# print(any([int(x) == 794.0 for x in input]))

print(best_individual.toString())

best_individual.set_cng("increment", 1)
best_individual.set_rng("random", 1)
best_individual.maxProcessTime = 10000
best_individual.registerLength = 10
best_individual.compile = True
print("===== END =====")