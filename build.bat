@echo off

cls

rmdir /s /q build

mkdir build || (
    echo Failed to create build dir!
    EXIT /B 1
)

cmake -B build -G "Ninja" -DCMAKE_C_COMPILER="C:\Program Files\LLVm\bin\clang.exe" || (
    echo Failed to configure cfrds!
    EXIT /B 1
)

cmake --build build --config Release || (
    echo Failed to build cfrds!
    EXIT /B 1
)
