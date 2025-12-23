@echo off

cls

rmdir /s /q %~dp0build %~dp0libxml2 %~dp0deps 2>nul

git clone  -bv2.15.1 --depth 1 https://github.com/GNOME/libxml2.git %~dp0libxml2

cmake -S %~dp0libxml2 -B %~dp0build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="C:\Program Files\LLVm\bin\clang.exe" -DLIBXML2_WITH_CATALOG=OFF -DLIBXML2_WITH_DEBUG=OFF -DLIBXML2_WITH_HTML=OFF -DLIBXML2_WITH_ISO8859X=OFF -DLIBXML2_WITH_MODULES=OFF -DLIBXML2_WITH_PATTERN=OFF -DLIBXML2_WITH_PUSH=OFF -DLIBXML2_WITH_REGEXPS=OFF -DLIBXML2_WITH_TESTS=OFF -DLIBXML2_WITH_THREADS=OFF -DLIBXML2_WITH_VALID=OFF -DLIBXML2_WITH_XINCLUDE=OFF -DLIBXML2_WITH_XPATH=OFF -DLIBXML2_WITH_ICONV=OFF -DBUILD_STATIC_LIBS=OFF
cmake --build %~dp0build --target LibXml2

mkdir %~dp0..\deps\libxml2\include\libxml 2>nul
mkdir %~dp0..\bin 2>nul

copy /y %~dp0libxml2\include\libxml %~dp0..\deps\libxml2\include\libxml
copy /y %~dp0build\libxml\xmlversion.h %~dp0..\deps\libxml2\include\libxml
copy /y %~dp0build\libxml2.dll %~dp0..\bin
copy /y %~dp0build\libxml2.lib %~dp0..\deps\libxml2\xml2.lib

rmdir /s /q %~dp0build %~dp0libxml2 2>nul
