#!/usr/bin/env bash

set -e

# Fuzzing uses libFuzzer (-fsanitize=fuzzer), which requires Clang.
CLANG_C="${CLANG_C:-clang}"
CLANG_CXX="${CLANG_CXX:-clang++}"

if ! command -v "$CLANG_C" >/dev/null 2>&1 || ! command -v "$CLANG_CXX" >/dev/null 2>&1; then
    echo "error: fuzzing requires Clang ('$CLANG_C' / '$CLANG_CXX' not found)" >&2
    exit 1
fi

rm -rf build

cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER="$CLANG_C" \
    -DCMAKE_CXX_COMPILER="$CLANG_CXX" \
    -DCFRDS_BUILD_FUZZ=ON
cmake --build build --config Debug
