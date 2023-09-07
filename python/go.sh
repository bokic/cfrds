#!/bin/bash -e

clear

rm -rf *.so

python3 setup.py build_ext --include-dirs=../include --build-lib=. && rm -rf build && python3 test.py
