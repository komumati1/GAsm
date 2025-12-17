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

def gen_binop_sum_sub_mul(op, n_random=1200, buf=10, seed=0):
    """
    1.2.C / 1.2.D / 1.2.E
    a,b w [-9999, 9999]
    io = [a, b, SENT, ..., SENT]
    target = [f(a,b)]
    """
    rng = random.Random(seed)
    lo, hi = -10, 10
    edges = [lo, lo+1, -1, 0, 1, hi-1, hi]

    cases = []
    for a in edges:
        for b in edges:
            cases.append((a, b))
    for _ in range(n_random):
        cases.append((rng.randint(lo, hi), rng.randint(lo, hi)))

    inputs, targets = [], []
    for a, b in cases:
        io = [float(a), float(b)] + [SENT] * (buf - 2)
        if op == "sum":
            y = a + b
        elif op == "sub":
            y = a - b
        elif op == "mul":
            y = a * b
        else:
            raise ValueError(op)

        inputs.append(io)
        targets.append([float(y)])
    return inputs, targets

def gen_max_dataset(lo=0, hi=9, n=200, buf=10, seed=0, include_edges=True):
    """
    Generuje dane do zadania max(a,b):
    - a,b w [lo, hi]
    - n losowych przykładów
    - wejście:  [a, b, SENT, ..., SENT] (długość buf)
    - target:   [max(a,b)]
    """
    rng = random.Random(seed)

    inputs, targets = [], []

    # opcjonalnie: dorzuć "trudne" przypadki brzegowe
    if include_edges:
        edges = [lo, lo + 1, -1, 0, 1, hi - 1, hi]
        # zostaw tylko te, które mieszczą się w [lo, hi]
        edges = [x for x in edges if lo <= x <= hi]
        for a in edges:
            for b in edges:
                io = [float(a), float(b)] + [SENT] * (buf - 2)
                inputs.append(io)
                targets.append([float(max(a, b))])

    # losowe przypadki
    for _ in range(n):
        a = rng.randint(lo, hi)
        b = rng.randint(lo, hi)
        io = [float(a), float(b)] + [SENT] * (buf - 2)
        inputs.append(io)
        targets.append([float(max(a, b))])

    return inputs, targets



import random
import math

SENT = 1234567.89

def round_half_up(x: float) -> int:
    # klasyczne zaokrąglenie: 1.5->2, -1.5->-1
    return int(math.floor(x + 0.5)) if x >= 0 else int(math.ceil(x - 0.5))

def gen_mean10_dataset(lo=-99, hi=99, n=500, buf=10, seed=0, include_edges=True):
    """
    1.4.A
    - wejście: 10 pierwszych liczb w [lo, hi]
    - target: zaokrąglona średnia (int) -> [float(int)]
    - io: [x0..x9] + [SENT]*(buf-10)
    """
    assert buf >= 10, "buf musi być >= 10"
    rng = random.Random(seed)

    inputs, targets = [], []

    # Edge cases (opcjonalnie): skrajne i mieszane wartości
    if include_edges:
        edge_vectors = [
            [lo]*10,
            [hi]*10,
            [0]*10,
            [lo, hi]*5,
            [hi, lo]*5,
            [-1, 1]*5,
            [1, -1]*5,
            ]
        for vec in edge_vectors:
            m = sum(vec) / 10.0
            y = round_half_up(m)
            io = [float(v) for v in vec] + [SENT] * (buf - 10)
            inputs.append(io)
            targets.append([float(y)])

    # Losowe przypadki
    for _ in range(n):
        vec = [rng.randint(lo, hi) for _ in range(10)]
        m = sum(vec) / 10.0
        y = round_half_up(m)

        io = [float(v) for v in vec] + [SENT] * (buf - 10)
        inputs.append(io)
        targets.append([float(y)])

    return inputs, targets