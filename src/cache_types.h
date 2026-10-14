#pragma once

#include <cstdint>

enum class ReplPolicy { LRU, FIFO };
enum class WritePolicy { WRITE_BACK, WRITE_THROUGH };

struct CacheConfig {
    uint64_t cache_size = 32 * 1024;
    uint32_t block_size = 64;
    uint32_t assoc = 4;
    ReplPolicy repl = ReplPolicy::LRU;
    WritePolicy write = WritePolicy::WRITE_BACK;
    double hit_time = 1.0;
    double miss_penalty = 100.0;
};

struct CacheStats {
    uint64_t reads = 0, writes = 0;
    uint64_t read_hits = 0, write_hits = 0;
    uint64_t evictions = 0;
    uint64_t writebacks = 0;
    uint64_t mem_reads = 0;
    uint64_t mem_writes = 0;

    uint64_t accesses() const { return reads + writes; }
    uint64_t hits() const { return read_hits + write_hits; }
    uint64_t misses() const { return accesses() - hits(); }

    double miss_rate() const {
        if (accesses() == 0) return 0.0;
        return double(misses()) / double(accesses());
    }
};
