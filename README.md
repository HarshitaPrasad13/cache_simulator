# Configurable Cache Simulator (C++)

A trace-driven, single-level, set-associative cache simulator.

- **Geometry:** cache size, block size, associativity (1 = direct-mapped, 1 set = fully associative)
- **Replacement:** LRU, FIFO
- **Write policy:** write-back + write-allocate, or write-through + no-write-allocate
- **Metrics:** hit/miss rate, AMAT, evictions, writebacks, memory reads/writes

## Layout

```
src/cache_types.h        config + stats data types
src/cache_line.h         one line: valid / dirty / tag / stamp
src/replacement_policy.* ReplacementPolicy base class, LruPolicy, FifoPolicy (add new policies here)
src/cache_set.*          CacheSet: lookup and fill within one set
src/cache.*              Cache: address split, hit/miss, write policy, statistics
src/main.cpp             CLI: reads a trace file, prints a report or one CSV line
src/test_cache.cpp       hand-computed unit tests (conflicts, LRU vs FIFO, write policies)
scripts/gen_trace.py     deterministic synthetic traces (sequential, stride, random, hotset, matrix row/col, conflict)
scripts/sweep.py         runs 448 configurations -> results.csv
scripts/cross_check.py   independent Python model; compares against the C++ on random cases
```

## Build and run

```bash
make                         # builds cachesim and test_cache
make test                    # unit tests
python3 scripts/gen_trace.py # creates traces/*.trace
python3 scripts/cross_check.py
python3 scripts/sweep.py     # writes results.csv

./cachesim --trace traces/matrix_col.trace --size 32768 --block 64 --assoc 4 --repl lru --write wb
```

Trace format: one access per line, `R 0x1234` or `W 0x1234`.

## Design

`Cache` owns a `ReplacementPolicy` and a vector of `CacheSet`s; each set owns its `CacheLine`s.
LRU and FIFO share one victim rule (smallest stamp) and differ only in when the stamp is refreshed,
so a new policy is one small subclass. The class split was a refactor: the 448-config sweep output
is byte-identical before and after, and the cross-check still reports 0 mismatches.

## How it works

An address is split into `| tag | set index | block offset |`. The index picks a set; the tag is compared
against every way in that set. A match is a hit. On a miss the simulator picks a victim (an empty way if
there is one, else the LRU or FIFO line), writes it back if dirty, and fills the new block.

- **LRU** evicts the line with the oldest *last use* (hits refresh it).
- **FIFO** evicts the line with the oldest *fill time* (hits do not refresh it).
- **AMAT** = hit_time + miss_rate x miss_penalty, using assumed values (1 and 100 cycles), not measured ones.

## Validation

1. Unit tests with expected values worked out by hand, including a sequence that separates LRU from FIFO.
2. `cross_check.py`: a separate Python implementation compared on 293 random configurations and traces
   (hits, misses, evictions, writebacks, memory traffic). Result: 0 mismatches.

Limits: this catches bugs in one implementation but not a misunderstanding shared by both. It has not
been compared against an external tool such as Valgrind's cachegrind.

## Results (synthetic traces, 32 KiB cache, 64 B blocks, LRU, miss rate %)

| trace | 1-way | 2-way | 4-way | 8-way |
|---|---|---|---|---|
| sequential | 6.25 | 6.25 | 6.25 | 6.25 |
| matrix_row | 6.25 | 6.25 | 6.25 | 6.25 |
| matrix_col | 100.00 | 100.00 | 100.00 | 100.00 |
| conflict | 100.00 | 100.00 | ~0 | ~0 |
| hotset | 18.94 | 22.25 | 24.61 | 25.79 |

What these show:
- **Block size** sets the spatial-locality benefit: sequential miss rate is 25% / 12.5% / 6.25% / 3.1% for 16 / 32 / 64 / 128 B blocks.
- **Row vs column traversal** of the same 256x256 int matrix: 6.25% vs 100% misses. The column stride (1 KiB) is a
  power of two, so the rows collide in a few sets and thrash even at 8-way.
- **Associativity** helps only when a set is oversubscribed: the `conflict` trace cycles 3 blocks that share one set,
  so 1- and 2-way miss every time while 4-way keeps all three.
- **hotset gets worse as associativity rises** in this run. This is a result of the trace and LRU, not a bug
  (the cross-check agrees). Work out why before you quote it: one cold block can trigger a chain of hot-line misses.

These are synthetic traces, so they illustrate behaviour; they are not a benchmark of real programs.

## Ideas to extend

- Add an L2 level; report global vs local miss rate.
- Classify misses as compulsory / capacity / conflict (3C model).
- Add real traces (e.g., from Valgrind's `lackey` tool) and a random or pseudo-LRU policy.
- Count a final flush of dirty lines so write-back traffic is complete.
