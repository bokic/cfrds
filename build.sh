#!/usr/bin/env bash

set -e

rm -rf build

BUILD_TESTING=OFF
if [ "$1" == "WITH_TESTS" ]; then
    BUILD_TESTING=ON
fi

cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=$BUILD_TESTING
cmake --build build --config Release
