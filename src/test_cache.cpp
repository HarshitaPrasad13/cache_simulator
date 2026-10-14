#include <cassert>
#include <cstdio>
#include <vector>

#include "cache.h"

static CacheConfig make_cfg(uint64_t size, uint32_t block, uint32_t assoc,
                            ReplPolicy repl = ReplPolicy::LRU,
                            WritePolicy write = WritePolicy::WRITE_BACK) {
    CacheConfig c;
    c.cache_size = size;
    c.block_size = block;
    c.assoc = assoc;
    c.repl = repl;
    c.write = write;
    return c;
}

static void test_direct_mapped_conflict() {
    Cache c(make_cfg(64, 16, 1));
    assert(!c.access(0x00, false));
    assert(c.access(0x04, false));
    assert(!c.access(0x40, false));
    assert(!c.access(0x00, false));
    assert(c.stats().hits() == 1 && c.stats().misses() == 3);
    assert(c.stats().evictions == 2);
}

static void test_lru_vs_fifo() {
    const uint64_t A = 0x00, B = 0x10, C = 0x20;

    Cache lru(make_cfg(32, 16, 2, ReplPolicy::LRU));
    lru.access(A, false);
    lru.access(B, false);
    lru.access(A, false);
    lru.access(C, false);
    lru.access(B, false);
    assert(lru.stats().hits() == 1 && lru.stats().misses() == 4);

    Cache fifo(make_cfg(32, 16, 2, ReplPolicy::FIFO));
    fifo.access(A, false);
    fifo.access(B, false);
    fifo.access(A, false);
    fifo.access(C, false);
    fifo.access(B, false);
    assert(fifo.stats().hits() == 2 && fifo.stats().misses() == 3);
}

static void test_write_back() {
    Cache c(make_cfg(16, 16, 1, ReplPolicy::LRU, WritePolicy::WRITE_BACK));
    assert(!c.access(0x00, true));
    assert(c.access(0x00, true));
    assert(c.stats().mem_writes == 0);
    assert(!c.access(0x10, false));
    assert(c.stats().writebacks == 1 && c.stats().mem_writes == 1);
}

static void test_write_through() {
    Cache c(make_cfg(16, 16, 1, ReplPolicy::LRU, WritePolicy::WRITE_THROUGH));
    assert(!c.access(0x00, true));
    assert(!c.access(0x00, false));
    assert(c.access(0x00, true));
    assert(c.stats().mem_writes == 2);
    assert(c.stats().writebacks == 0);
}

static void test_lru_thrash() {
    Cache c(make_cfg(64, 16, 4));
    for (int round = 0; round < 3; ++round) {
        for (uint64_t block = 0; block < 5; ++block) c.access(block * 16, false);
    }
    assert(c.stats().hits() == 0);
}

static void test_cache_set_directly() {
    LruPolicy lru;
    CacheSet set(2, &lru);

    assert(set.find(1) == nullptr);
    assert(!set.fill(1, false, 1).evicted);
    assert(!set.fill(2, true, 2).evicted);

    set.touch(*set.find(1), 3);
    CacheSet::FillResult r = set.fill(3, false, 4);

    assert(r.evicted && r.was_dirty);
    assert(set.find(2) == nullptr && set.find(1) && set.find(3));
}

static void test_policies_directly() {
    std::vector<CacheLine> ways(2);
    ways[0].stamp = 1;
    ways[1].stamp = 2;

    LruPolicy lru;
    FifoPolicy fifo;

    fifo.on_hit(ways[0], 10);
    assert(ways[0].stamp == 1);
    assert(fifo.pick_victim(ways) == 0);

    lru.on_hit(ways[0], 10);
    assert(ways[0].stamp == 10);
    assert(lru.pick_victim(ways) == 1);
}

static void test_bad_geometry() {
    bool threw = false;
    try {
        Cache c(make_cfg(100, 16, 1));
    } catch (...) {
        threw = true;
    }
    assert(threw);
}

int main() {
    test_direct_mapped_conflict();
    test_lru_vs_fifo();
    test_write_back();
    test_write_through();
    test_lru_thrash();
    test_cache_set_directly();
    test_policies_directly();
    test_bad_geometry();
    std::puts("All cache tests passed.");
    return 0;
}
