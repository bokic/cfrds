@echo off
setlocal EnableDelayedExpansion

REM usage: build_zip.bat [x64|arm64]  (no argument builds zips for both arches)

set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=both"

if /i "%ARCH%"=="both" (
    for %%A in (x64 arm64) do (
        call :make_zip %%A
        if errorlevel 1 (
            echo [ERROR] zip failed for %%A
            exit /b 1
        )
    )
) else (
    if /i "%ARCH%"=="x64" (
        call :make_zip %ARCH%
    ) else if /i "%ARCH%"=="arm64" (
        call :make_zip %ARCH%
    ) else (
        echo [ERROR] Invalid arch "%ARCH%". Use x64 or arm64.
        exit /b 1
    )
    if errorlevel 1 (
        echo [ERROR] zip failed for %ARCH%
        exit /b 1
    )
)

echo [OK] zip (%ARCH%) done.
exit /b 0

:make_zip
set "ZIP_ARCH=%~1"

for /f "delims=" %%i in ('git describe --tags --dirty') do set git_describe=%%i

mkdir cfrds\include
copy ..\bin\%ZIP_ARCH%\cfrds.exe cfrds
copy ..\bin\%ZIP_ARCH%\cfrds.dll cfrds
copy ..\bin\%ZIP_ARCH%\cfrds.lib cfrds
copy ..\bin\%ZIP_ARCH%\json-c.dll cfrds
copy ..\bin\%ZIP_ARCH%\libxml2.dll cfrds
copy ..\include\cfrds.h cfrds\include
copy ..\include\cfrds.hpp cfrds\include

tar -a -c -f "cfrds-%git_describe%-%ZIP_ARCH%.zip" "cfrds\cfrds.exe" "cfrds\cfrds.dll" "cfrds\cfrds.lib" "cfrds\json-c.dll" "cfrds\libxml2.dll" "cfrds\include\cfrds.h" "cfrds\include\cfrds.hpp"
rmdir /s /q cfrds

exit /b 0