#!/bin/bash -e

clear

clang -c -flto -Os -fPIC -I/usr/include/python3.11 -I../include cfrds.py.c ../src/cfrds.c ../src/cfrds_buffer.c ../src/cfrds_http.c
clang -flto -Os -s cfrds.py.o cfrds.o cfrds_buffer.o cfrds_http.o -lpython3.11 -shared -o cfrds.so
chmod -x cfrds.so
rm *.o

python test.py
