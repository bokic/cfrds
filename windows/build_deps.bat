@echo off

cls

SET JSON_C_VERSION=0.18-20240915
SET LIBXML2_VERSION=2.15.1

rmdir /s /q  json-c pcre2 build deps

mkdir ..\bin

curl -L --output json-c.zip https://github.com/json-c/json-c/archive/refs/tags/json-c-%JSON_C_VERSION%.zip
tar -xf json-c.zip
del json-c.zip

cmake.exe -S json-c-json-c-%JSON_C_VERSION% -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=OFF -DBUILD_TESTING=OFF -DBUILD_APPS=OFF
cmake.exe --build build --config Release

mkdir ..\deps\json-c\include

copy /y json-c-json-c-%JSON_C_VERSION%\*.h ..\deps\json-c\include
copy /y build\*.h ..\deps\json-c\include

xcopy /y /e build\Release\json-c.dll ..\bin
xcopy /y /e build\Release\json-c.lib ..\deps\json-c

rmdir /s /q build json-c-json-c-%JSON_C_VERSION%

curl -L --output libxml2.zip https://github.com/GNOME/libxml2/archive/refs/tags/v%LIBXML2_VERSION%.zip
tar -xf libxml2.zip
del libxml2.zip

cmake.exe -S libxml2-%LIBXML2_VERSION% -B build -DCMAKE_BUILD_TYPE=Release -DLIBXML2_WITH_CATALOG=OFF -DLIBXML2_WITH_DEBUG=OFF -DLIBXML2_WITH_HTML=OFF -DLIBXML2_WITH_ISO8859X=OFF -DLIBXML2_WITH_MODULES=OFF -DLIBXML2_WITH_PATTERN=OFF -DLIBXML2_WITH_PUSH=OFF -DLIBXML2_WITH_REGEXPS=OFF -DLIBXML2_WITH_TESTS=OFF -DLIBXML2_WITH_THREADS=OFF -DLIBXML2_WITH_VALID=OFF -DLIBXML2_WITH_XINCLUDE=OFF -DLIBXML2_WITH_XPATH=OFF -DLIBXML2_WITH_ICONV=OFF -DBUILD_STATIC_LIBS=OFF
cmake.exe --build build --config Release --target LibXml2

mkdir ..\deps\libxml2\include\libxml 2>nul
mkdir ..\bin 2>nul

copy /y libxml2-%LIBXML2_VERSION%\include\libxml ..\deps\libxml2\include\libxml
copy /y build\libxml\xmlversion.h ..\deps\libxml2\include\libxml
copy /y build\Release\libxml2.dll ..\bin
copy /y build\Release\libxml2.lib ..\deps\libxml2\xml2.lib

rmdir /s /q build libxml2-%LIBXML2_VERSION%
