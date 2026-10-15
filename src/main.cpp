#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <string>

#include "cache.h"

typedef unsigned long long ull;

// Convert a number to text. We print these with %s instead of %llu because
// the old Windows C library (MinGW) does not understand %llu.
static std::string num(ull v) { return std::to_string(v); }

static void print_usage(const char* prog) {
    std::fprintf(stderr,
        "Usage: %s --trace FILE [options]\n"
        "  --size BYTES        total cache size        (default 32768)\n"
        "  --block BYTES       block size              (default 64)\n"
        "  --assoc N           ways per set            (default 4)\n"
        "  --repl lru|fifo     replacement policy      (default lru)\n"
        "  --write wb|wt       write-back / write-through (default wb)\n"
        "  --hit-time CYC      hit time for AMAT       (default 1)\n"
        "  --miss-penalty CYC  miss penalty for AMAT   (default 100)\n"
        "  --csv               print one machine-readable line instead of a report\n",
        prog);
}

static void print_csv(const Cache& cache) {
    const CacheStats& s = cache.stats();
    std::printf("%s,%s,%s,%.6f,%.4f,%s,%s,%s,%s\n",
                num(s.accesses()).c_str(), num(s.hits()).c_str(), num(s.misses()).c_str(),
                s.miss_rate(), cache.amat(),
                num(s.evictions).c_str(), num(s.writebacks).c_str(),
                num(s.mem_reads).c_str(), num(s.mem_writes).c_str());
}

static void print_report(const Cache& cache) {
    const CacheConfig& cfg = cache.config();
    const CacheStats& s = cache.stats();

    const char* repl = (cfg.repl == ReplPolicy::LRU) ? "LRU" : "FIFO";
    const char* write = (cfg.write == WritePolicy::WRITE_BACK)
                            ? "write-back/write-allocate"
                            : "write-through/no-write-allocate";

    std::printf("Config: %s B, %u B blocks, %u-way, %s sets, %s, %s\n",
                num(cfg.cache_size).c_str(), (unsigned)cfg.block_size, (unsigned)cfg.assoc,
                num(cache.num_sets()).c_str(), repl, write);
    std::printf("Accesses : %s (reads %s, writes %s)\n",
                num(s.accesses()).c_str(), num(s.reads).c_str(), num(s.writes).c_str());
    std::printf("Hits     : %s\nMisses   : %s\n", num(s.hits()).c_str(), num(s.misses()).c_str());
    std::printf("Miss rate: %.4f%%\n", 100.0 * s.miss_rate());
    std::printf("AMAT     : %.3f cycles (hit %.1f + miss rate * penalty %.1f)\n",
                cache.amat(), cfg.hit_time, cfg.miss_penalty);
    std::printf("Evictions: %s, writebacks: %s\n", num(s.evictions).c_str(), num(s.writebacks).c_str());
    std::printf("Memory   : %s block reads, %s write ops\n",
                num(s.mem_reads).c_str(), num(s.mem_writes).c_str());
}

int main(int argc, char** argv) {
    CacheConfig cfg;
    std::string trace_path;
    bool csv = false;

    for (int i = 1; i < argc; ++i) {
        std::string flag = argv[i];

        auto value = [&]() -> const char* {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                std::exit(2);
            }
            return argv[++i];
        };

        if (flag == "--trace") trace_path = value();
        else if (flag == "--size") cfg.cache_size = std::strtoull(value(), nullptr, 0);
        else if (flag == "--block") cfg.block_size = std::strtoul(value(), nullptr, 0);
        else if (flag == "--assoc") cfg.assoc = std::strtoul(value(), nullptr, 0);
        else if (flag == "--repl")
            cfg.repl = std::strcmp(value(), "fifo") == 0 ? ReplPolicy::FIFO : ReplPolicy::LRU;
        else if (flag == "--write")
            cfg.write = std::strcmp(value(), "wt") == 0 ? WritePolicy::WRITE_THROUGH
                                                        : WritePolicy::WRITE_BACK;
        else if (flag == "--hit-time") cfg.hit_time = std::atof(value());
        else if (flag == "--miss-penalty") cfg.miss_penalty = std::atof(value());
        else if (flag == "--csv") csv = true;
        else {
            print_usage(argv[0]);
            return 2;
        }
    }

    if (trace_path.empty()) {
        print_usage(argv[0]);
        return 2;
    }

    try {
        Cache cache(cfg);

        std::ifstream trace(trace_path);
        if (!trace) {
            std::fprintf(stderr, "cannot open %s\n", trace_path.c_str());
            return 1;
        }

        std::string line;
        while (std::getline(trace, line)) {
            if (line.empty() || line[0] == '#') continue;

            char op = 0;
            char addr_text[64];
            if (std::sscanf(line.c_str(), " %c %63s", &op, addr_text) != 2) continue;

            ull addr = std::strtoull(addr_text, nullptr, 0);
            cache.access(addr, op == 'W' || op == 'w');
        }

        if (csv) print_csv(cache);
        else print_report(cache);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
