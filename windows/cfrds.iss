#define AppVer GetVersionNumbersString('..\bin\cfrds.exe')

[Setup]
AppName=cfrds
AppVerName=cfrds version {#AppVer}
AppPublisher=Boris Barbulovski
AppPublisherURL=https://github.com/bokic/cfrds
AppVersion={#AppVer}
ArchitecturesInstallIn64BitMode=x64os
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
Source: "..\bin\cfrds.exe"; DestDir: "{app}";
Source: "..\bin\cfrds.dll"; DestDir: "{app}";
Source: "..\bin\json-c.dll"; DestDir: "{app}";
Source: "..\bin\libxml2.dll"; DestDir: "{app}";
