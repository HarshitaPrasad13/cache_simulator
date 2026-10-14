#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <string>

#include "cache.h"

typedef unsigned long long ull;

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
    std::printf("%llu,%llu,%llu,%.6f,%.4f,%llu,%llu,%llu,%llu\n",
                (ull)s.accesses(), (ull)s.hits(), (ull)s.misses(),
                s.miss_rate(), cache.amat(),
                (ull)s.evictions, (ull)s.writebacks,
                (ull)s.mem_reads, (ull)s.mem_writes);
}

static void print_report(const Cache& cache) {
    const CacheConfig& cfg = cache.config();
    const CacheStats& s = cache.stats();

    const char* repl = (cfg.repl == ReplPolicy::LRU) ? "LRU" : "FIFO";
    const char* write = (cfg.write == WritePolicy::WRITE_BACK)
                            ? "write-back/write-allocate"
                            : "write-through/no-write-allocate";

    std::printf("Config: %llu B, %u B blocks, %u-way, %llu sets, %s, %s\n",
                (ull)cfg.cache_size, cfg.block_size, cfg.assoc,
                (ull)cache.num_sets(), repl, write);
    std::printf("Accesses : %llu (reads %llu, writes %llu)\n",
                (ull)s.accesses(), (ull)s.reads, (ull)s.writes);
    std::printf("Hits     : %llu\nMisses   : %llu\n", (ull)s.hits(), (ull)s.misses());
    std::printf("Miss rate: %.4f%%\n", 100.0 * s.miss_rate());
    std::printf("AMAT     : %.3f cycles (hit %.1f + miss rate * penalty %.1f)\n",
                cache.amat(), cfg.hit_time, cfg.miss_penalty);
    std::printf("Evictions: %llu, writebacks: %llu\n", (ull)s.evictions, (ull)s.writebacks);
    std::printf("Memory   : %llu block reads, %llu write ops\n",
                (ull)s.mem_reads, (ull)s.mem_writes);
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
