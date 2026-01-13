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


import random

SENT = 1234567.89

def gen_min4_dataset(lo=-99, hi=99, n=2000, buf=10, seed=0, include_edges=True):
    """
    Wejście:  io = [a, b, c, d, SENT, ..., SENT]
    Target:   [min(a,b,c,d)]
    """
    assert buf >= 4
    rng = random.Random(seed)

    inputs, targets = [], []

    if include_edges:
        edges = [lo, lo+1, -1, 0, 1, hi-1, hi]
        edges = [x for x in edges if lo <= x <= hi]

        # kilka ręcznych, mocno informacyjnych układów
        edge_quads = [
            [lo, lo, lo, lo],
            [hi, hi, hi, hi],
            [lo, hi, hi, hi],
            [hi, lo, hi, hi],
            [hi, hi, lo, hi],
            [hi, hi, hi, lo],
            [0, 0, 0, 0],
            [0, 1, 2, 3],
            [3, 2, 1, 0],
            [-1, 0, 1, 2],
            [2, 1, 0, -1],
        ]

        # wszystkie kombinacje z listy edges (uwaga: rośnie szybko – zostawiamy małą listę edges)
        for a in edges:
            for b in edges:
                for c in edges:
                    for d in edges:
                        quad = [a, b, c, d]
                        y = min(quad)
                        io = [float(x) for x in quad] + [SENT] * (buf - 4)
                        inputs.append(io)
                        targets.append([float(y)])

        for quad in edge_quads:
            y = min(quad)
            io = [float(x) for x in quad] + [SENT] * (buf - 4)
            inputs.append(io)
            targets.append([float(y)])

    # losowe przypadki
    for _ in range(n):
        quad = [rng.randint(lo, hi) for _ in range(4)]
        y = min(quad)
        io = [float(x) for x in quad] + [SENT] * (buf - 4)
        inputs.append(io)
        targets.append([float(y)])

    return inputs, targets



SENT = 1234567.89

def gen_neg_to_zero_dataset(L=8, lo=-99, hi=99, n=2000, buf=10, seed=0, include_edges=True):
    """
    Zadanie: wszystkie ujemne -> 0, reszta bez zmian.
    Wejście:  io[0..L-1] = liczby, io[L..] = SENT
    Target:   tgt[0..L-1] = max(xi,0)  (trzymamy jako lista długości L)
    """
    assert buf >= L
    rng = random.Random(seed)

    inputs, targets = [], []

    if include_edges:
        edge_vecs = [
            [-1]*L,
            [0]*L,
            [1]*L,
            [lo]*L,
            [hi]*L,
            [-1, 0, 1] * (L // 3) + ([-1] if L % 3 else []),
            [lo, -1, 0, 1, hi] * (L // 5) + ([0] * (L % 5)),
            ]
        for vec in edge_vecs:
            vec = vec[:L]
            tgt = [float(max(v, 0)) for v in vec]
            io = [float(v) for v in vec] + [SENT] * (buf - L)
            inputs.append(io)
            targets.append(tgt)

    for _ in range(n):
        vec = [rng.randint(lo, hi) for _ in range(L)]
        # lekko zwiększ częstość liczb ujemnych i zera (pomaga nauczyć regułę)
        if rng.random() < 0.3:
            j = rng.randrange(L)
            vec[j] = -rng.randint(1, abs(lo))
        if rng.random() < 0.2:
            j = rng.randrange(L)
            vec[j] = 0

        tgt = [float(max(v, 0)) for v in vec]
        io = [float(v) for v in vec] + [SENT] * (buf - L)
        inputs.append(io)
        targets.append(tgt)

    return inputs, targets




SENT = 1234567.89

def all_binary_inputs(k: int):
    # zwraca listę list bitów długości k, w kolejności 0..2^k-1
    res = []
    for mask in range(1 << k):
        vec = [(mask >> i) & 1 for i in range(k)]  # D0 najmłodszy bit
        res.append(vec)
    return res

def gen_boolean_dataset(k: int, func, buf=12, seed=0, add_constants=True):
    """
    func: callable(bits:list[int]) -> 0/1
    inputs: [D0..Dk-1, (opcjonalnie 1,0), SENT...]
    targets: [[f(D)]]
    """
    extra = 2 if add_constants else 0
    assert buf >= k + extra

    inputs, targets = [], []
    for bits in all_binary_inputs(k):
        io = [float(b) for b in bits]
        if add_constants:
            io += [1.0, 0.0]
        io += [SENT] * (buf - len(io))

        y = int(func(bits))  # 0/1
        inputs.append(io)
        targets.append([float(y)])
    return inputs, targets


def make_random_truth_table(k: int, seed=0):
    rng = random.Random(seed)
    table = [rng.randint(0, 1) for _ in range(1 << k)]
    def f(bits):
        idx = sum((bits[i] << i) for i in range(k))
        return table[idx]
    return f, table

# przykład:
# f_rand, table = make_random_truth_table(5, seed=123)
# inputs, targets = gen_boolean_dataset(5, f_rand, buf=16, add_constants=True)



SENT_INT = 0  #0

def gen_arith_sequence_dataset(
        n=2000,
        Tmax=5,
        start_lo=1, start_hi=5,
        step_lo=1, step_hi=3,
        seed=0,
        buf=None
):
    """
    Problem: start, end, step -> wypisz start, start+step, ... dla n_i < end
    Konwencja:
      io[0]=start, io[1]=end, io[2]=step
      io[3..3+Tmax-1] = kolejne elementy (reszta ma zostać SENT_INT)
    targets: lista długości Tmax, gdzie:
      target[j] = wartość ciągu dla j < L, a dla j>=L -> SENT_INT
    """
    rng = random.Random(seed)
    if buf is None:
        buf = 3 + Tmax  # minimum
    assert buf >= 3 + Tmax

    inputs, targets = [], []

    # edge cases: różne długości i wartości brzegowe
    for L in [0, 1, 2, Tmax-1, Tmax]:
        start = start_lo
        step = step_lo
        end = start + step * L
        io = [float(start), float(end), float(step)] + [float(SENT_INT)] * Tmax
        io += [0.0] * (buf - (3 + Tmax))  # scratch = 0.0 (nie sentinel)
        tgt = [float(start + step*j) for j in range(L)] + [float(SENT_INT)] * (Tmax - L)
        inputs.append(io)
        targets.append(tgt)

    for _ in range(n):
        start = rng.randint(start_lo, start_hi)
        step = rng.randint(step_lo, step_hi)          # step>0
        L = rng.randint(0, Tmax)                      # liczba elementów do wypisania (<=Tmax)
        end = start + step * L                        # gwarantuje dokładnie L elementów < end

        io = [float(start), float(end), float(step)] + [float(SENT_INT)] * Tmax
        io += [0.0] * (buf - (3 + Tmax))

        tgt = [float(start + step*j) for j in range(L)] + [float(SENT_INT)] * (Tmax - L)

        inputs.append(io)
        targets.append(tgt)

    return inputs, targets