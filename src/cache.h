#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "cache_set.h"
#include "cache_types.h"
#include "replacement_policy.h"

class Cache {
public:
    explicit Cache(const CacheConfig& cfg);

    bool access(uint64_t addr, bool is_write);

    const CacheStats& stats() const { return stats_; }
    const CacheConfig& config() const { return cfg_; }
    uint64_t num_sets() const { return sets_.size(); }
    double amat() const { return cfg_.hit_time + stats_.miss_rate() * cfg_.miss_penalty; }

private:
    CacheConfig cfg_;
    CacheStats stats_;
    uint32_t offset_bits_ = 0;
    uint32_t index_bits_ = 0;
    uint64_t tick_ = 0;
    std::unique_ptr<ReplacementPolicy> policy_;
    std::vector<CacheSet> sets_;
};
