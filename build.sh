#!env bash

set -e

cmake -B build
cmake --build build

cp build/compile_commands.json .
