@echo off

cls

rmdir /s /q build
mkdir "build"

"C:\Program Files\CMake\bin\cmake.exe" -G "Ninja" -DCMAKE_MAKE_PROGRAM:FILEPATH="C:\Program Files\CMake\bin\ninja.exe" -B build -DCMAKE_C_COMPILER="C:\Program Files\LLVm\bin\clang.exe"
"C:\Program Files\CMake\bin\ninja.exe" -C build
