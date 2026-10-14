#include "cache.h"

#include <stdexcept>

namespace {

bool is_power_of_two(uint64_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}

uint32_t log2_of(uint64_t x) {
    uint32_t bits = 0;
    while (x > 1) {
        x >>= 1;
        ++bits;
    }
    return bits;
}

}

Cache::Cache(const CacheConfig& cfg) : cfg_(cfg) {
    if (!is_power_of_two(cfg.cache_size) || !is_power_of_two(cfg.block_size) ||
        !is_power_of_two(cfg.assoc)) {
        throw std::invalid_argument("cache_size, block_size and assoc must be powers of two");
    }
    if (cfg.cache_size < uint64_t(cfg.block_size) * cfg.assoc) {
        throw std::invalid_argument("cache_size must hold at least one full set");
    }

    uint64_t set_count = cfg.cache_size / (uint64_t(cfg.block_size) * cfg.assoc);
    offset_bits_ = log2_of(cfg.block_size);
    index_bits_ = log2_of(set_count);

    policy_ = make_policy(cfg.repl);
    sets_.reserve(set_count);
    for (uint64_t i = 0; i < set_count; ++i) {
        sets_.emplace_back(cfg.assoc, policy_.get());
    }
}

bool Cache::access(uint64_t addr, bool is_write) {
    ++tick_;
    if (is_write) ++stats_.writes;
    else ++stats_.reads;

    uint64_t index = (addr >> offset_bits_) & (sets_.size() - 1);
    uint64_t tag = addr >> (offset_bits_ + index_bits_);
    CacheSet& set = sets_[index];

    if (CacheLine* line = set.find(tag)) {
        set.touch(*line, tick_);
        if (!is_write) {
            ++stats_.read_hits;
        } else {
            ++stats_.write_hits;
            if (cfg_.write == WritePolicy::WRITE_BACK) line->dirty = true;
            else ++stats_.mem_writes;
        }
        return true;
    }

    if (is_write && cfg_.write == WritePolicy::WRITE_THROUGH) {
        ++stats_.mem_writes;
        return false;
    }

    ++stats_.mem_reads;
    CacheSet::FillResult result = set.fill(tag, is_write, tick_);
    if (result.evicted) {
        ++stats_.evictions;
        if (result.was_dirty) {
            ++stats_.writebacks;
            ++stats_.mem_writes;
        }
    }
    return false;
}
