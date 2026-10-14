CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

CORE = src/cache.cpp src/cache_set.cpp src/replacement_policy.cpp
HDRS = $(wildcard src/*.h)

all: cachesim test_cache

cachesim: src/main.cpp $(CORE) $(HDRS)
	$(CXX) $(CXXFLAGS) src/main.cpp $(CORE) -o $@

# -UNDEBUG keeps assert() active even though -O2 is on
test_cache: src/test_cache.cpp $(CORE) $(HDRS)
	$(CXX) $(CXXFLAGS) -UNDEBUG src/test_cache.cpp $(CORE) -o $@

test: test_cache
	./test_cache

clean:
	rm -f cachesim test_cache results.csv
