#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "cache_line.h"
#include "cache_types.h"

class ReplacementPolicy {
public:
    virtual ~ReplacementPolicy() = default;

    virtual void on_fill(CacheLine& line, uint64_t tick) const { line.stamp = tick; }
    virtual void on_hit(CacheLine& line, uint64_t tick) const = 0;
    virtual uint32_t pick_victim(const std::vector<CacheLine>& ways) const;
};

class LruPolicy : public ReplacementPolicy {
public:
    void on_hit(CacheLine& line, uint64_t tick) const override { line.stamp = tick; }
};

class FifoPolicy : public ReplacementPolicy {
public:
    void on_hit(CacheLine&, uint64_t) const override {}
};

std::unique_ptr<ReplacementPolicy> make_policy(ReplPolicy kind);
