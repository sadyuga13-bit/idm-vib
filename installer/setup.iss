; IDM Vibe - Native C++20 Multi-Segment & YouTube Download Engine
; Inno Setup 6 Professional Windows x64 Installer Script

#define MyAppName "IDM Vibe Download Engine"
#define MyAppVersion "2.0.0"
#define MyAppPublisher "SystemsArchitecture Engineering"
#define MyAppExeName "IdmVibe.exe"

[Setup]
AppId={{8F9E2A41-6C15-4D73-9B2E-51A480C9E712}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\IdmVibe
DefaultGroupName={#MyAppName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=IdmVibe-Setup-x64-v{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
CloseApplications=yes
RestartApplications=no
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "arabic"; MessagesFile: "compiler:Languages\Arabic.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "autostart"; Description: "Launch IDM Vibe automatically at Windows startup (System Tray)"; GroupDescription: "System Integration:"

[Dirs]
Name: "{usercfgs}\..\Downloads\Programs"
Name: "{usercfgs}\..\Downloads\Compressed"
Name: "{usercfgs}\..\Downloads\Video"
Name: "{usercfgs}\..\Downloads\Music"
Name: "{usercfgs}\..\Downloads\Documents"

[Files]
Source: "..\build\Release\IdmVibe.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\build\Release\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\build\Release\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\build\Release\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\build\Release\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\build\Release\sqldrivers\*"; DestDir: "{app}\sqldrivers"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\build\Release\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\chrome-extension\*"; DestDir: "{app}\chrome-extension"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Chrome Extension Folder"; Filename: "{app}\chrome-extension"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "IdmVibe"; ValueData: """{app}\{#MyAppExeName}"" --tray"; Flags: uninsdeletevalue; Tasks: autostart

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{localappdata}\SystemsArchitecture\IdmVibe\temp_parts"
