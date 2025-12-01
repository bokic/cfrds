#!env bash

set -e

mkdir -p build
cmake -B build
cmake --build build
