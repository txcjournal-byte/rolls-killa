; Inno Setup script for HAT GOBLIN by TrapWin (Windows, VST3 + Standalone)
; Build: ISCC.exe /DAppVersion=0.1.0 installer\win\hatgoblin.iss
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

[Setup]
AppId={{6B0D3F2E-91A4-4C7B-8E52-3A9F1C7D2B64}
AppName=HAT GOBLIN by TrapWin
AppVersion={#AppVersion}
AppPublisher=TrapWin
AppPublisherURL=https://trapwin.com
DefaultDirName={commonpf64}\TrapWin\HAT GOBLIN
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
LicenseFile=..\EULA.txt
OutputDir=..\..\build\installer
OutputBaseFilename=HAT-GOBLIN-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes

[Files]
Source: "..\..\build\HatGoblin_artefacts\Release\VST3\HAT GOBLIN.vst3\*"; DestDir: "{commoncf64}\VST3\HAT GOBLIN.vst3"; Excludes: "*.pdb,*.ilk"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\build\HatGoblin_artefacts\Release\Standalone\HAT GOBLIN.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\EULA.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\HAT GOBLIN"; Filename: "{app}\HAT GOBLIN.exe"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\HAT GOBLIN.vst3"
