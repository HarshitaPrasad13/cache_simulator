#!/usr/bin/env python3
"""Run the simulator over many configurations and write results.csv.

Usage:  python3 scripts/sweep.py            (after `make` and `python3 scripts/gen_trace.py`)
"""
import csv
import itertools
import os
import subprocess

ROOT = os.path.join(os.path.dirname(__file__), "..")
SIM = os.path.join(ROOT, "cachesim")
TRACES = ["sequential", "stride", "random", "hotset", "matrix_row", "matrix_col", "conflict"]
SIZES = [8 * 1024, 32 * 1024]
BLOCKS = [16, 32, 64, 128]
ASSOCS = [1, 2, 4, 8]
REPLS = ["lru", "fifo"]
FIELDS = ["accesses", "hits", "misses", "miss_rate", "amat",
          "evictions", "writebacks", "mem_reads", "mem_writes"]


def run(trace, size, block, assoc, repl, write="wb"):
    out = subprocess.run(
        [SIM, "--trace", os.path.join(ROOT, "traces", trace + ".trace"),
         "--size", str(size), "--block", str(block), "--assoc", str(assoc),
         "--repl", repl, "--write", write, "--csv"],
        capture_output=True, text=True, check=True).stdout.strip()
    return dict(zip(FIELDS, out.split(",")))


def main():
    rows = []
    for t, s, b, a, r in itertools.product(TRACES, SIZES, BLOCKS, ASSOCS, REPLS):
        res = run(t, s, b, a, r)
        rows.append({"trace": t, "size": s, "block": b, "assoc": a, "repl": r, **res})
    path = os.path.join(ROOT, "results.csv")
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} configurations -> {os.path.abspath(path)}")

    # Small console summary: miss rate (%) vs associativity, 32 KiB / 64 B / LRU.
    print("\nMiss rate (%) vs associativity  [32 KiB, 64 B blocks, LRU]")
    print(f"{'trace':<12}" + "".join(f"{str(a)+'-way':>9}" for a in ASSOCS))
    for t in TRACES:
        line = f"{t:<12}"
        for a in ASSOCS:
            r = next(x for x in rows if x["trace"] == t and x["size"] == 32768
                     and x["block"] == 64 and x["assoc"] == a and x["repl"] == "lru")
            line += f"{100 * float(r['miss_rate']):>9.2f}"
        print(line)


if __name__ == "__main__":
    main()
