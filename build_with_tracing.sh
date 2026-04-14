#!/usr/bin/env bash

set -e

rm -rf build

cmake -B build -DENABLE_PERFETTO=ON
cmake --build build
