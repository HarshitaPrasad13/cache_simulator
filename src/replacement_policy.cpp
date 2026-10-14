#include "replacement_policy.h"

uint32_t ReplacementPolicy::pick_victim(const std::vector<CacheLine>& ways) const {
    uint32_t victim = 0;
    for (uint32_t i = 1; i < ways.size(); ++i) {
        if (ways[i].stamp < ways[victim].stamp) victim = i;
    }
    return victim;
}

std::unique_ptr<ReplacementPolicy> make_policy(ReplPolicy kind) {
    if (kind == ReplPolicy::FIFO) return std::make_unique<FifoPolicy>();
    return std::make_unique<LruPolicy>();
}
