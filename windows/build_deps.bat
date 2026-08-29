@echo off

SET JSON_C_VERSION=0.19-20260627
SET LIBXML2_VERSION=2.15.2

REM "Visual Studio 18 2026" generator requires CMake >= 4.2
where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake.exe not found in PATH.
    exit /b 1
)

for /f "tokens=3" %%v in ('cmake.exe --version') do (
    set CMAKE_VERSION=%%v
    goto cmake_version_parsed
)
:cmake_version_parsed
for /f "tokens=1,2 delims=." %%a in ("%CMAKE_VERSION%") do (
    set CMAKE_VERSION_MAJOR=%%a
    set CMAKE_VERSION_MINOR=%%b
)

if %CMAKE_VERSION_MAJOR% LSS 4 goto cmake_too_old
if %CMAKE_VERSION_MAJOR% EQU 4 if %CMAKE_VERSION_MINOR% LSS 2 goto cmake_too_old
echo [OK] Using CMake %CMAKE_VERSION%
goto cmake_done

:cmake_too_old
echo [ERROR] CMake %CMAKE_VERSION% detected. The "Visual Studio 18 2026" generator requires CMake 4.2 or newer.
echo         Upgrade with: winget install Kitware.CMake
exit /b 1

:cmake_done

rmdir /s /q %~dp0json-c %~dp0pcre2 %~dp0build %~dp0deps 2>nul

curl -L --output json-c.zip https://github.com/json-c/json-c/archive/refs/tags/json-c-%JSON_C_VERSION%.zip
tar -xf json-c.zip
del json-c.zip

cmake.exe -S json-c-json-c-%JSON_C_VERSION% -B build -G "Visual Studio 18 2026" -A x64 -DBUILD_STATIC_LIBS=OFF -DBUILD_TESTING=OFF -DBUILD_APPS=OFF
cmake.exe --build build --config Release --target json-c

mkdir ..\deps\json-c\include 2>nul
mkdir ..\bin 2>nul

copy /y json-c-json-c-%JSON_C_VERSION%\*.h ..\deps\json-c\include
copy /y build\*.h ..\deps\json-c\include

copy /y build\Release\json-c.dll ..\bin
copy /y build\Release\json-c.lib ..\deps\json-c

rmdir /s /q build json-c-json-c-%JSON_C_VERSION%

curl -L --output libxml2.zip https://github.com/GNOME/libxml2/archive/refs/tags/v%LIBXML2_VERSION%.zip
tar -xf libxml2.zip
del libxml2.zip

cmake.exe -S libxml2-%LIBXML2_VERSION% -B build -G "Visual Studio 18 2026" -A x64 -DLIBXML2_WITH_CATALOG=OFF -DLIBXML2_WITH_DEBUG=OFF -DLIBXML2_WITH_HTML=OFF -DLIBXML2_WITH_ISO8859X=OFF -DLIBXML2_WITH_MODULES=OFF -DLIBXML2_WITH_PATTERN=OFF -DLIBXML2_WITH_PUSH=OFF -DLIBXML2_WITH_REGEXPS=OFF -DLIBXML2_WITH_TESTS=OFF -DLIBXML2_WITH_THREADS=OFF -DLIBXML2_WITH_VALID=OFF -DLIBXML2_WITH_XINCLUDE=OFF -DLIBXML2_WITH_XPATH=OFF -DLIBXML2_WITH_ICONV=OFF
cmake.exe --build build --config Release --target LibXml2

mkdir ..\deps\libxml2\include\libxml 2>nul
mkdir ..\bin 2>nul

copy /y libxml2-%LIBXML2_VERSION%\include\libxml\*.h ..\deps\libxml2\include\libxml
copy /y build\libxml\xmlversion.h ..\deps\libxml2\include\libxml
copy /y build\Release\libxml2.dll ..\bin
copy /y build\Release\libxml2.lib ..\deps\libxml2\xml2.lib

rmdir /s /q build libxml2-%LIBXML2_VERSION%
