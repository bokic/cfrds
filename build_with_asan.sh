#!/usr/bin/env bash

set -e

rm -rf build

cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCFRDS_SANITIZE=ON
cmake --build build --config Debug
