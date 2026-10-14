#!/usr/bin/env python3
"""Generate synthetic memory-access traces (deterministic, seeded).

Each trace line is:  R|W 0xADDRESS

Patterns (chosen because each stresses a different cache behaviour):
  sequential   - walk memory linearly            -> spatial locality, compulsory misses
  stride       - fixed 256 B jumps               -> poor spatial locality, conflict-prone
  random       - uniform over a 4 MiB region     -> almost no locality
  hotset       - 90% accesses to a small hot set -> temporal locality
  matrix_row   - sum a 256x256 int matrix row-major    (cache friendly)
  matrix_col   - same matrix, column-major             (cache hostile)
  conflict     - cycle through 3 blocks 32 KiB apart   (all map to ONE set in any cache
                 <= 32 KiB, so it needs >= 3 ways to avoid conflict misses)
"""
import os
import random

OUT = os.path.join(os.path.dirname(__file__), "..", "traces")
N = 200_000          # accesses per synthetic trace
WRITE_FRAC = 0.25    # share of accesses that are writes


def emit(path, accesses):
    with open(path, "w") as f:
        for is_write, addr in accesses:
            f.write(f"{'W' if is_write else 'R'} 0x{addr:08x}\n")


def sequential(rng):
    return [(rng.random() < WRITE_FRAC, 0x10000 + 4 * i) for i in range(N)]


def stride(rng):
    return [(rng.random() < WRITE_FRAC, 0x10000 + (256 * i) % (1 << 20)) for i in range(N)]


def uniform_random(rng):
    return [(rng.random() < WRITE_FRAC, 0x10000 + (rng.randrange(1 << 22) & ~3)) for _ in range(N)]


def hotset(rng):
    hot = 0x10000 + 4 * 1024 * 8       # 32 KiB hot region (fits in a 32 KiB cache only barely)
    cold = 1 << 22
    out = []
    for _ in range(N):
        if rng.random() < 0.9:
            addr = 0x10000 + (rng.randrange(8 * 1024) * 4) % (hot - 0x10000)
        else:
            addr = 0x10000 + (rng.randrange(cold) & ~3)
        out.append((rng.random() < WRITE_FRAC, addr))
    return out


def matrix(row_major):
    n, base, elem = 256, 0x100000, 4
    out = []
    for a in range(n):
        for b in range(n):
            i, j = (a, b) if row_major else (b, a)
            out.append((False, base + (i * n + j) * elem))   # read A[i][j]
    return out


def conflict(rng):
    blocks = [0x10000 + k * 32 * 1024 for k in range(3)]
    return [(rng.random() < WRITE_FRAC, blocks[i % 3]) for i in range(N)]


def main():
    os.makedirs(OUT, exist_ok=True)
    rng = random.Random(12345)
    emit(os.path.join(OUT, "sequential.trace"), sequential(rng))
    emit(os.path.join(OUT, "stride.trace"), stride(rng))
    emit(os.path.join(OUT, "random.trace"), uniform_random(rng))
    emit(os.path.join(OUT, "hotset.trace"), hotset(rng))
    emit(os.path.join(OUT, "matrix_row.trace"), matrix(True))
    emit(os.path.join(OUT, "matrix_col.trace"), matrix(False))
    emit(os.path.join(OUT, "conflict.trace"), conflict(rng))
    print("traces written to", os.path.abspath(OUT))


if __name__ == "__main__":
    main()
