#!/usr/bin/env bash

set -e

rm -rf build

cmake -B build -DBUILD_TESTING=ON -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic"
cmake --build build

./bin/test_buffer
./bin/test_wddx
