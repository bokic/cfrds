@echo off
setlocal EnableDelayedExpansion

REM usage: build_deps.bat [x64|arm64]  (no argument builds both arches)

set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=both"

SET JSON_C_VERSION=0.19-20260627
SET LIBXML2_VERSION=2.15.2

REM Ensure cmake.exe is present (all arches use the Ninja generator)
where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake.exe not found in PATH.
    exit /b 1
)

REM json-c needs these regardless of arch (it builds with -Werror otherwise).
set "JSON_ARGS=-DDISABLE_WERROR=ON -DSIZEOF_SSIZE_T=8"

REM Download and extract the sources once; both arches share them.
rmdir /s /q "%~dp0json-c-json-c-%JSON_C_VERSION%" 2>nul

REM The json-c archive contains tests/*.test symlinks which need elevation to
REM extract on Windows; they are not needed for the library, so exclude them.
curl -L --output json-c.zip https://github.com/json-c/json-c/archive/refs/tags/json-c-%JSON_C_VERSION%.zip
tar --exclude=*/tests/*.test -xf json-c.zip
del json-c.zip

curl -L --output libxml2.zip https://github.com/GNOME/libxml2/archive/refs/tags/v%LIBXML2_VERSION%.zip
tar -xf libxml2.zip
del libxml2.zip

if /i "%ARCH%"=="both" (
    for %%A in (x64 arm64) do (
        call :build_arch %%A
        if errorlevel 1 (
            echo [ERROR] deps build failed for %%A
            exit /b 1
        )
    )
) else (
    call :build_arch %ARCH%
    if errorlevel 1 (
        echo [ERROR] deps build failed for %ARCH%
        exit /b 1
    )
)

rmdir /s /q "%~dp0json-c-json-c-%JSON_C_VERSION%" 2>nul
rmdir /s /q "%~dp0libxml2-%LIBXML2_VERSION%" 2>nul

echo [OK] deps (%ARCH%) built.
exit /b 0

:build_arch
set "ARCH=%~1"

REM Both arches use clang (Ninja) - the same toolchain as build.bat.
REM Avoids the "Visual Studio 18 2026" generator entirely, which can hang
REM waiting on MSBuild/VS component dialogs.
if /i "%ARCH%"=="x64" (
    set "EXTRA_ARGS="
) else (
    REM arm64 is cross-compiled with clang (Ninja)
    set "EXTRA_ARGS=-DCMAKE_C_COMPILER_TARGET=aarch64-pc-windows-msvc -DCMAKE_SYSTEM_PROCESSOR=ARM64"
)

set "JSON_BUILD=%~dp0deps-build-%ARCH%-json"
set "XML_BUILD=%~dp0deps-build-%ARCH%-xml"

rmdir /s /q "%JSON_BUILD%" 2>nul
rmdir /s /q "%XML_BUILD%" 2>nul

cmake.exe -S "%~dp0json-c-json-c-%JSON_C_VERSION%" -B "%JSON_BUILD%" -G Ninja -DCMAKE_C_COMPILER="C:\Program Files\LLVM\bin\clang.exe" -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=OFF -DBUILD_TESTING=OFF -DBUILD_APPS=OFF %JSON_ARGS% %EXTRA_ARGS%
if errorlevel 1 (
    echo [ERROR] Failed to configure json-c for %ARCH%
    exit /b 1
)
cmake.exe --build "%JSON_BUILD%" --target json-c
if errorlevel 1 (
    echo [ERROR] Failed to build json-c for %ARCH%
    exit /b 1
)

mkdir "..\deps\%ARCH%\json-c\include" 2>nul
mkdir "..\bin\%ARCH%" 2>nul

xcopy /y "%~dp0json-c-json-c-%JSON_C_VERSION%\*.h" "..\deps\%ARCH%\json-c\include" >nul
xcopy /y "%JSON_BUILD%\*.h" "..\deps\%ARCH%\json-c\include" >nul
xcopy /y "%JSON_BUILD%\json-c.dll" "..\bin\%ARCH%" >nul
xcopy /y "%JSON_BUILD%\json-c.lib" "..\deps\%ARCH%\json-c" >nul

rmdir /s /q "%JSON_BUILD%" 2>nul

cmake.exe -S "%~dp0libxml2-%LIBXML2_VERSION%" -B "%XML_BUILD%" -G Ninja -DCMAKE_C_COMPILER="C:\Program Files\LLVM\bin\clang.exe" -DCMAKE_BUILD_TYPE=Release -DLIBXML2_WITH_CATALOG=OFF -DLIBXML2_WITH_DEBUG=OFF -DLIBXML2_WITH_HTML=OFF -DLIBXML2_WITH_ISO8859X=OFF -DLIBXML2_WITH_MODULES=OFF -DLIBXML2_WITH_PATTERN=OFF -DLIBXML2_WITH_PUSH=OFF -DLIBXML2_WITH_REGEXPS=OFF -DLIBXML2_WITH_TESTS=OFF -DLIBXML2_WITH_THREADS=OFF -DLIBXML2_WITH_VALID=OFF -DLIBXML2_WITH_XINCLUDE=OFF -DLIBXML2_WITH_XPATH=OFF -DLIBXML2_WITH_ICONV=OFF %EXTRA_ARGS%
if errorlevel 1 (
    echo [ERROR] Failed to configure libxml2 for %ARCH%
    exit /b 1
)
cmake.exe --build "%XML_BUILD%" --target LibXml2
if errorlevel 1 (
    echo [ERROR] Failed to build libxml2 for %ARCH%
    exit /b 1
)

mkdir "..\deps\%ARCH%\libxml2\include\libxml" 2>nul

xcopy /y /e "%~dp0libxml2-%LIBXML2_VERSION%\include\libxml" "..\deps\%ARCH%\libxml2\include\libxml" >nul
xcopy /y "%XML_BUILD%\libxml\xmlversion.h" "..\deps\%ARCH%\libxml2\include\libxml" >nul
xcopy /y "%XML_BUILD%\libxml2.dll" "..\bin\%ARCH%" >nul
copy /y "%XML_BUILD%\libxml2.lib" "..\deps\%ARCH%\libxml2\xml2.lib" >nul

rmdir /s /q "%XML_BUILD%" 2>nul

echo [OK] deps (%ARCH%) built.
exit /b 0
