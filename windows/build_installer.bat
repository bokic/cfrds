@echo off

for /f "delims=" %%i in ('git rev-parse --short HEAD') do set git_hash=%%i
for /f "delims=" %%i in ('git describe --tags --dirty') do set git_describe=%%i

>installer.ini (
	echo [installer]
	echo target_name = "cfrds_%git_describe%"
	echo description = "ColdFusion RDS tool(#%git_hash%)"
)

"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" "cfrds.iss"

del installer.ini
