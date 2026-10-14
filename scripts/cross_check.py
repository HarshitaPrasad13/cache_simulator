#!/usr/bin/env python3
"""Independent validation: re-implement the cache in ~40 lines of Python and
compare it with the C++ simulator on many random configurations and traces.

If both implementations agree on hits, misses, evictions, writebacks and memory
traffic for every case, a logic bug that exists in only one of them is very unlikely.
(Caveat: a *misunderstanding* of cache behaviour shared by both would not be caught.)

Usage:  python3 scripts/cross_check.py
"""
import os
import random
import subprocess
import tempfile

ROOT = os.path.join(os.path.dirname(__file__), "..")
SIM = os.path.join(ROOT, "cachesim")


def reference(trace, size, block, assoc, repl, write):
    """Plain-Python model. Each set is an ordered list of [tag, dirty]; index 0 is the victim."""
    nsets = size // (block * assoc)
    sets = [[] for _ in range(nsets)]
    hits = misses = evict = wb = mem_r = mem_w = 0
    for is_write, addr in trace:
        blk = addr // block
        idx, tag = blk % nsets, blk // nsets
        s = sets[idx]
        hit = next((e for e in s if e[0] == tag), None)
        if hit:
            hits += 1
            if repl == "lru":            # LRU: move to most-recent end; FIFO: leave order alone
                s.remove(hit); s.append(hit)
            if is_write:
                if write == "wb": hit[1] = True
                else: mem_w += 1
            continue
        misses += 1
        if is_write and write == "wt":   # no-write-allocate
            mem_w += 1
            continue
        if len(s) == assoc:              # set full -> evict element 0
            v = s.pop(0)
            evict += 1
            if v[1]:
                wb += 1; mem_w += 1
        s.append([tag, is_write and write == "wb"])
        mem_r += 1
    return [len(trace), hits, misses, evict, wb, mem_r, mem_w]


def cpp(path, size, block, assoc, repl, write):
    out = subprocess.run([SIM, "--trace", path, "--size", str(size), "--block", str(block),
                          "--assoc", str(assoc), "--repl", repl, "--write", write, "--csv"],
                         capture_output=True, text=True, check=True).stdout.strip().split(",")
    # accesses,hits,misses,miss_rate,amat,evictions,writebacks,mem_reads,mem_writes
    return [int(out[0]), int(out[1]), int(out[2]), int(out[5]), int(out[6]), int(out[7]), int(out[8])]


def main():
    rng = random.Random(7)
    cases = fails = 0
    for _ in range(300):
        size = rng.choice([256, 1024, 4096, 16384])
        block = rng.choice([16, 32, 64])
        assoc = rng.choice([1, 2, 4, 8])
        if size < block * assoc:
            continue
        repl, write = rng.choice(["lru", "fifo"]), rng.choice(["wb", "wt"])
        # Small address space + repeated hot blocks forces plenty of conflicts and reuse.
        span = rng.choice([512, 4096, 65536])
        trace = [(rng.random() < 0.3, rng.randrange(span)) for _ in range(rng.randrange(50, 1500))]
        with tempfile.NamedTemporaryFile("w", suffix=".trace", delete=False) as f:
            for w, a in trace:
                f.write(f"{'W' if w else 'R'} {a}\n")
            path = f.name
        a, b = cpp(path, size, block, assoc, repl, write), reference(trace, size, block, assoc, repl, write)
        os.unlink(path)
        cases += 1
        if a != b:
            fails += 1
            print("MISMATCH", dict(size=size, block=block, assoc=assoc, repl=repl, write=write), a, b)
    print(f"{cases} random cases compared, {fails} mismatches")
    raise SystemExit(1 if fails else 0)


if __name__ == "__main__":
    main()
