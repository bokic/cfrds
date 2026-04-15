#!/usr/bin/env bash

set -e

rm -rf build

cmake -B build -DENABLE_PERFETTO=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build  --config Debug
