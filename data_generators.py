import random

SENT = 1234567.89

def gen_11_anypos_constant(C, n=200, buf=10, seed=0):
    rng = random.Random(seed)
    inputs, targets = [], []
    for _ in range(n):
        # dowolne wartości startowe
        io = [rng.randint(-9, 9) for _ in range(buf)]
        # możesz też część zrobić sentinelami:
        # io = [rng.randint(-9, 9)] + [SENT]*(buf-1)
        inputs.append(io)
        targets.append([float(C)])
    return inputs, targets