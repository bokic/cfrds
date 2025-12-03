@echo off

cls

for /f "delims=" %%i in ('git describe --tags --dirty') do set git_describe=%%i

rmdir /s /q build

mkdir build || (
    echo Failed to create build dir!
    EXIT /B 1
)

cmake -B build -G "Ninja" -DCMAKE_C_COMPILER="C:\Program Files\LLVm\bin\clang.exe" -DCMAKE_BUILD_TYPE=Release || (
    echo Failed to configure cfrds!
    EXIT /B 1
)

cmake --build build || (
    echo Failed to build cfrds!
    EXIT /B 1
)

mkdir cfrds
copy bin\cfrds.exe cfrds
copy bin\cfrds.dll cfrds
copy bin\libxml2.dll cfrds

tar -a -c -f "cfrds-%git_describe%.zip" "cfrds\cfrds.exe" "cfrds\cfrds.dll" "cfrds\libxml2.dll"
rmdir /s /q cfrds
