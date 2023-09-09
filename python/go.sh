#!/bin/bash -e

clear

rm -rf *.so

python3 setup.py build_ext --build-lib=. && rm -rf build && python3 test.py
