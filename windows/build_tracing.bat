@echo off
setlocal EnableDelayedExpansion

REM usage: build_tracing.bat [x64|arm64]  (no argument builds both arches)

cls

set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=both"

if /i "%ARCH%"=="both" (
    for %%A in (x64 arm64) do (
        call :build_arch %%A
        if errorlevel 1 (
            echo [ERROR] build_tracing failed for %%A
            exit /b 1
        )
    )
) else (
    if /i "%ARCH%"=="x64" (
        call :build_arch %ARCH%
    ) else if /i "%ARCH%"=="arm64" (
        call :build_arch %ARCH%
    ) else (
        echo [ERROR] Invalid arch "%ARCH%". Use x64 or arm64.
        exit /b 1
    )
    if errorlevel 1 (
        echo [ERROR] build_tracing failed for %ARCH%
        exit /b 1
    )
)

echo [OK] build_tracing (%ARCH%) done.
exit /b 0

:build_arch
set "BUILD_ARCH=%~1"
set "BUILD_DIR=%~dp0build-%BUILD_ARCH%"

rmdir /s /q "%BUILD_DIR%" 2>nul

mkdir "%BUILD_DIR%" || (
    echo Failed to create build dir for %BUILD_ARCH%!
    exit /b 1
)

if /i "%BUILD_ARCH%"=="x64" (
    set "EXTRA_ARGS="
) else (
    set "EXTRA_ARGS=-DCMAKE_C_COMPILER_TARGET=aarch64-pc-windows-msvc -DCMAKE_SYSTEM_PROCESSOR=ARM64"
)

cmake -S %~dp0.. -B "%BUILD_DIR%" -G "Ninja" -DCMAKE_C_COMPILER="C:\Program Files\LLVm\bin\clang.exe" -DCMAKE_CXX_COMPILER="C:\Program Files\LLVm\bin\clang++.exe" -DCFRDS_ARCH=%BUILD_ARCH% -DENABLE_PERFETTO=ON -DCMAKE_BUILD_TYPE=Release %EXTRA_ARGS% || (
    echo Failed to configure cfrds for %BUILD_ARCH%!
    exit /b 1
)

cmake --build "%BUILD_DIR%" || (
   echo Failed to build cfrds for %BUILD_ARCH%!
   exit /b 1
)

rmdir /s /q "%BUILD_DIR%" 2>nul

exit /b 0