#pragma once

#include <cstdint>
#include <vector>

#include "cache_line.h"
#include "replacement_policy.h"

class CacheSet {
public:
    struct FillResult {
        bool evicted = false;
        bool was_dirty = false;
    };

    CacheSet(uint32_t ways, const ReplacementPolicy* policy);

    CacheLine* find(uint64_t tag);
    void touch(CacheLine& line, uint64_t tick);
    FillResult fill(uint64_t tag, bool dirty, uint64_t tick);

private:
    std::vector<CacheLine> ways_;
    const ReplacementPolicy* policy_;
};
