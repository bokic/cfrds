#define AppVer GetVersionNumbersString('..\bin\x64\cfrds.exe')

[Setup]
AppName=cfrds
AppVerName=cfrds version {#AppVer}
AppPublisher=Boris Barbulovski
AppPublisherURL=https://github.com/bokic/cfrds
AppVersion={#AppVer}
; "ArchitecturesInstallIn64BitMode=x64compatible or arm64" instructs
; Setup to use "64-bit install mode" on x64-compatible systems and
; Arm64 systems, meaning Setup should use the native 64-bit Program
; Files directory and the 64-bit view of the registry. On all other
; OS architectures (e.g., 32-bit x86), Setup will use "32-bit
; install mode".
ArchitecturesInstallIn64BitMode=x64compatible or arm64
DefaultDirName={commonpf}\cfrds
DefaultGroupName=cfrds
UninstallDisplayIcon={app}\cfrds.exe
Compression=lzma
SolidCompression=yes
OutputBaseFilename={#ReadIni(SourcePath + "installer.ini", "installer", "target_name ")}
OutputDir=.
VersionInfoVersion={#AppVer}
VersionInfoDescription={#ReadIni(SourcePath + "installer.ini", "installer", "description")}

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Flags: preservestringtype

[Setup]
ChangesEnvironment=yes

[Dirs]
Name: "{app}";

[Files]
; ARM64 systems get the native ARM64 binaries.
Source: "..\bin\arm64\cfrds.exe"; DestDir: "{app}"; Check: PreferArm64Files; Flags: solidbreak
Source: "..\bin\arm64\cfrds.dll"; DestDir: "{app}"; Check: PreferArm64Files
Source: "..\bin\arm64\json-c.dll"; DestDir: "{app}"; Check: PreferArm64Files
Source: "..\bin\arm64\libxml2.dll"; DestDir: "{app}"; Check: PreferArm64Files
; x64-compatible systems get the x64 binaries.
Source: "..\bin\x64\cfrds.exe"; DestDir: "{app}"; Check: PreferX64Files; Flags: solidbreak
Source: "..\bin\x64\cfrds.dll"; DestDir: "{app}"; Check: PreferX64Files
Source: "..\bin\x64\json-c.dll"; DestDir: "{app}"; Check: PreferX64Files
Source: "..\bin\x64\libxml2.dll"; DestDir: "{app}"; Check: PreferX64Files

[Code]
function PreferArm64Files: Boolean;
begin
  Result := IsArm64;
end;

function PreferX64Files: Boolean;
begin
  Result := (not PreferArm64Files) and IsX64Compatible;
end;