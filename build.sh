#!/usr/bin/env sh
set -eu
CXX=${CXX:-clang++}
mkdir -p build
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude \
  src/karp_rabin.cpp src/boyer_moore.cpp tests/test_search.cpp \
  -o build/run_tests
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude \
  src/karp_rabin.cpp src/boyer_moore.cpp src/benchmark.cpp \
  -o build/benchmark
printf 'Built build/run_tests and build/benchmark with %s\n' "$CXX"
