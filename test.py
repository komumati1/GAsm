import gasm
from data_generators import gen_11_anypos_constant, gen_binop_sum_sub_mul, gen_boolean_dataset, make_random_truth_table

# Continue from file DON'T CHANGE POPULATION SIZE

# test_case = ""
test_case = "final_bool_04"
g = gasm.GAsm.fromJson(f"data/{test_case}.json")

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
k = 4
# inputs, targets = gen_binop_sum_sub_mul("mul", n_random=500, buf=10, seed=1)  # 1.2.C
f_rand, _ = make_random_truth_table(k, seed=100+k)
inputs, targets = gen_boolean_dataset(k, f_rand, buf=max(12, k+2), add_constants=True)

num = 3
SENT = 1234567.89

def test_model(test_case, num1, num2):
    SENT = 1234567.89
    g = gasm.GAsm.fromJson(f"data/example_{test_case}.json")
    hist = g.hist
    best_individual = g.best

    inputs = [SENT for _ in range(10)]

    inputs[0] = num1
    inputs[1] = num2

    time = best_individual.run(inputs)
    print(f"Result: {[1 if x >= 0.5 else 0 for x in inputs]}")
    print(best_individual.toString())


# input = [float(x) for x in inputs[num]]
# input = [float(0) for x in range(13)]
# input[0] = 37.0
# input[1] = -333.0
# input[2] = -33.0
# input[3] = -4444.0
# input[4] = 23.0
# input[9] = -SENT
# input[10] = -2.0
# input[12] = -1.0


print(inputs[num])
print(targets[num])
# print(inputs[0])
# input = [float(x) for x in range(100)]
# import random
# rng = random.Random(1)
# input = [float(rng.randint(-100, 100)) for _ in range(100)]
time = best_individual.run(inputs[num])
# print(f"Process time: {time}")
print(f"Result: {[1 if x > 0.5 else 0 for x in inputs[num]]}")
# print(f"Result: {[int(x) for x in inputs[num]]}")
# print(any([int(x) == 794.0 for x in input]))

# print(best_individual.toString())

best_individual.set_cng("increment", 1)
best_individual.set_rng("random", 1)
best_individual.maxProcessTime = 10000
best_individual.registerLength = 10
best_individual.compile = True
print("===== END =====")