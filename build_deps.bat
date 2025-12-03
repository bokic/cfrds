@echo off

cls

rmdir /s /q build libxml2 deps

git clone  -bv2.15.1 --depth 1 https://github.com/GNOME/libxml2.git

cmake -S libxml2 -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="C:\Program Files\LLVm\bin\clang.exe" -DLIBXML2_WITH_CATALOG=OFF -DLIBXML2_WITH_DEBUG=OFF -DLIBXML2_WITH_HTML=OFF -DLIBXML2_WITH_ISO8859X=OFF -DLIBXML2_WITH_MODULES=OFF -DLIBXML2_WITH_PATTERN=OFF -DLIBXML2_WITH_PUSH=OFF -DLIBXML2_WITH_REGEXPS=OFF -DLIBXML2_WITH_TESTS=OFF -DLIBXML2_WITH_THREADS=OFF -DLIBXML2_WITH_VALID=OFF -DLIBXML2_WITH_XINCLUDE=OFF -DLIBXML2_WITH_XPATH=OFF -DLIBXML2_WITH_ICONV=OFF -DBUILD_STATIC_LIBS=OFF
cmake --build build --target LibXml2

mkdir deps\libxml2\include
mkdir bin 2> NUL

move libxml2\include\libxml deps\libxml2\include
move build\libxml\xmlversion.h deps\libxml2\include\libxml
move build\libxml2.dll bin
move build\libxml2.lib deps\libxml2\xml2.lib

rmdir /s /q build libxml2
