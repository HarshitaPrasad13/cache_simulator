# Configurable Cache Simulator (C++)

A trace-driven, single-level, set-associative cache simulator.

- **Geometry:** cache size, block size, associativity (1 = direct-mapped, 1 set = fully associative)
- **Replacement:** LRU, FIFO
- **Write policy:** write-back + write-allocate, or write-through + no-write-allocate
- **Metrics:** hit/miss rate, AMAT, evictions, writebacks, memory reads/writes

Requirements: a C++17 compiler (`g++`), `make`, Python 3.

## Layout

```
src/cache_types.h        config + stats data types
src/cache_line.h         one line: valid / dirty / tag / stamp
src/replacement_policy.* ReplacementPolicy base class, LruPolicy, FifoPolicy (add new policies here)
src/cache_set.*          CacheSet: lookup and fill within one set
src/cache.*              Cache: address split, hit/miss, write policy, statistics
src/main.cpp             CLI: reads a trace file, prints a report or one CSV line
src/test_cache.cpp       hand-computed unit tests (conflicts, LRU vs FIFO, write policies, sets, policies)
scripts/gen_trace.py     deterministic synthetic traces (sequential, stride, random, hotset, matrix row/col, conflict)
scripts/sweep.py         runs 28 configurations (7 traces x 4 associativities, 32 KiB, 64 B blocks, LRU) -> results.csv
```

## Build and run

```bash
make                          # builds cachesim and test_cache
make test                     # unit tests
python3 scripts/gen_trace.py  # creates traces/*.trace
python3 scripts/sweep.py      # writes results.csv

./cachesim --trace traces/matrix_col.trace --size 32768 --block 64 --assoc 4 --repl lru --write wb
```

Options: `--trace FILE` (required), `--size BYTES`, `--block BYTES`, `--assoc N`, `--repl lru|fifo`,
`--write wb|wt`, `--hit-time CYCLES`, `--miss-penalty CYCLES`, `--csv` (one machine-readable line).
Size, block size and associativity must be powers of two.

Trace format: one access per line, `R 0x1234` or `W 0x1234`.

## Design

`Cache` owns a `ReplacementPolicy` and a vector of `CacheSet`s; each set owns its `CacheLine`s.
LRU and FIFO share one victim rule (smallest stamp) and differ only in when the stamp is refreshed
(LRU on every hit, FIFO only on fill), so a new policy is one small subclass.

## How it works

An address is split into `| tag | set index | block offset |`. The index picks a set; the tag is compared
against every way in that set. A match is a hit. On a miss the simulator picks a victim (an empty way if
there is one, else the LRU or FIFO line), writes it back if dirty, and fills the new block.

- **LRU** evicts the line with the oldest *last use* (hits refresh it).
- **FIFO** evicts the line with the oldest *fill time* (hits do not refresh it).
- **AMAT** = hit_time + miss_rate x miss_penalty, using assumed values (1 and 100 cycles), not measured ones.
- **Memory counters:** `mem_reads` counts blocks fetched. `mem_writes` counts memory write operations:
  dirty-block writebacks for write-back, every write for write-through. They are different units,
  so don't compare `mem_writes` across the two policies.

## Validation

Unit tests with expected values worked out by hand, including a sequence that separates LRU from FIFO,
and direct tests of `CacheSet` and the policies.

Limits: the tests cover only the cases worked out by hand. There is no second implementation or external
tool (such as Valgrind's cachegrind) to compare against.

## What it does not model

Data values (only tags and state), timing, multi-level hierarchies, prefetching, coherence. Dirty lines still
in the cache when the trace ends are not flushed, so `writebacks` and `mem_writes` for write-back runs
slightly undercount.

## Results (synthetic traces, 32 KiB cache, 64 B blocks, LRU, miss rate %)

| trace | 1-way | 2-way | 4-way | 8-way |
|---|---|---|---|---|
| sequential | 6.25 | 6.25 | 6.25 | 6.25 |
| matrix_row | 6.25 | 6.25 | 6.25 | 6.25 |
| matrix_col | 100.00 | 100.00 | 100.00 | 100.00 |
| conflict | 100.00 | 100.00 | 0.0015 | 0.0015 |
| hotset | 18.94 | 22.25 | 24.61 | 25.79 |

What these show:
- **Block size** sets the spatial-locality benefit: sequential miss rate is 25% / 12.5% / 6.25% / 3.1% for
  16 / 32 / 64 / 128 B blocks (single runs with `--block`, 32 KiB, same result at 1- and 4-way; not part of the sweep).
- **Row vs column traversal** of the same 256x256 int matrix: 6.25% vs 100% misses. The column stride (1 KiB) is a
  power of two, so the rows collide in a few sets and thrash even at 8-way.
- **Associativity** helps only when a set is oversubscribed: the `conflict` trace cycles 3 blocks that share one set,
  so 1- and 2-way miss every time (200,000 of 200,000) while 4-way misses only the 3 compulsory ones.
- **hotset gets worse as associativity rises** in this run. This is probably a result of the trace and LRU, not a bug,
  but it is not independently verified. Likely mechanism: the hot region exactly fills the cache, so each cold block
  takes a way and displaces a hot block; with higher associativity it can stay resident longer under LRU, squeezing
  out more hot blocks before it ages away.

These are synthetic traces, so they illustrate behaviour; they are not a benchmark of real programs.