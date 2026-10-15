# HOW TO READ THIS FILE
# A line starting with # is a comment (a note for humans, ignored by make).
# A Makefile is a list of jobs. Each job looks like this:
#
#   jobname:
#   <TAB>command to run
#
# You run a job by typing:  make jobname
# The command lines MUST start with a TAB character, not spaces.

# Job 1: build the simulator and the tests.
# Run it with:  make        (or:  make build)
# "g++" is the C++ compiler. It turns source code (.cpp files) into a
# program you can run. "-o cachesim" names the finished program "cachesim".
# "-O2" makes it fast. "-std=c++17" picks the C++ language version.
build:
	g++ -std=c++17 -O2 -Wall -o cachesim src/main.cpp src/cache.cpp src/cache_set.cpp src/replacement_policy.cpp
	g++ -std=c++17 -O2 -Wall -UNDEBUG -o test_cache src/test_cache.cpp src/cache.cpp src/cache_set.cpp src/replacement_policy.cpp

# Job 2: build, then run the unit tests.
# Run it with:  make test
# "build" after the colon means: do the build job first.
test: build
	./test_cache

# Job 3: delete everything the other jobs created.
# Run it with:  make clean
# WARNING: this also deletes results.csv. Run sweep.py again to recreate it.
clean:
	rm -f cachesim test_cache results.csv

# Tell make these names are jobs, not files, so they always run.
.PHONY: build test clean
