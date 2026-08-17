#!/usr/bin/env bash

set -e

rm -rf build

cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer -Wall -Wextra -Wpedantic"
cmake --build build --config Debug
