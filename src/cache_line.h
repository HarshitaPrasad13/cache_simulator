#pragma once

#include <cstdint>

struct CacheLine {
    bool valid = false;
    bool dirty = false;
    uint64_t tag = 0;
    uint64_t stamp = 0;
};
