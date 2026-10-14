#include "cache_set.h"

CacheSet::CacheSet(uint32_t ways, const ReplacementPolicy* policy)
    : ways_(ways), policy_(policy) {}

CacheLine* CacheSet::find(uint64_t tag) {
    for (CacheLine& line : ways_) {
        if (line.valid && line.tag == tag) return &line;
    }
    return nullptr;
}

void CacheSet::touch(CacheLine& line, uint64_t tick) {
    policy_->on_hit(line, tick);
}

CacheSet::FillResult CacheSet::fill(uint64_t tag, bool dirty, uint64_t tick) {
    FillResult result;
    CacheLine* slot = nullptr;

    for (CacheLine& line : ways_) {
        if (!line.valid) {
            slot = &line;
            break;
        }
    }

    if (slot == nullptr) {
        slot = &ways_[policy_->pick_victim(ways_)];
        result.evicted = true;
        result.was_dirty = slot->dirty;
    }

    slot->valid = true;
    slot->tag = tag;
    slot->dirty = dirty;
    policy_->on_fill(*slot, tick);
    return result;
}
