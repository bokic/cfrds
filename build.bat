@echo off

cls

rmdir /s /q %~dp0build 2>nul

mkdir %~dp0build || (
    echo Failed to create build dir!
    EXIT /B 1
)

cmake -S %~dp0 -B %~dp0build -G "Ninja" -DCMAKE_C_COMPILER="C:\Program Files\LLVm\bin\clang.exe" -DCMAKE_BUILD_TYPE=Release || (
    echo Failed to configure cfrds!
    EXIT /B 1
)

cmake --build %~dp0build || (
   echo Failed to build cfrds!
   EXIT /B 1
)

rmdir /s /q %~dp0build 2>nul
