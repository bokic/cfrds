@echo off

cls

for /f "delims=" %%i in ('git describe --tags --dirty') do set git_describe=%%i

mkdir cfrds\include
copy ..\bin\cfrds.exe cfrds
copy ..\bin\cfrds.dll cfrds
copy ..\bin\cfrds.lib cfrds
copy ..\bin\json-c.dll cfrds
copy ..\bin\libxml2.dll cfrds
copy ..\include\cfrds.h cfrds\include

tar -a -c -f "cfrds-%git_describe%.zip" "cfrds\cfrds.exe" "cfrds\cfrds.dll" "cfrds\cfrds.lib" "cfrds\json-c.dll" "cfrds\libxml2.dll" "cfrds\include\cfrds.h"
rmdir /s /q cfrds
