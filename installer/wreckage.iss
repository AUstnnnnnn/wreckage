; Windows installer for WRECKAGE. Built in CI: iscc /DSrc=<build artefacts dir> installer\wreckage.iss
#ifndef Src
  #define Src "..\build\Wreckage_artefacts\Release"
#endif

[Setup]
AppId={{6C1E0D29-2026-4A29-B0C7-57EC0AEC1029}
AppName=WRECKAGE
AppVersion=1.0.0
AppPublisher=Austin Radliff
AppComments=For WRECKEM. Oct 29 2026.
DefaultDirName={autopf}\WRECKAGE
DefaultGroupName=WRECKAGE
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=WRECKAGE-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
InfoAfterFile=..\INSTALL.txt
UninstallDisplayName=WRECKAGE

[Files]
; The VST3 goes where FL Studio and every other Windows host looks.
Source: "{#Src}\VST3\WRECKAGE.vst3\*"; DestDir: "{commoncf64}\VST3\WRECKAGE.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Src}\Standalone\WRECKAGE.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\INSTALL.txt"; DestDir: "{app}"; Flags: ignoreversion

[Dirs]
Name: "{userdocs}\WRECKAGE\Presets"; Flags: uninsneveruninstall

[Icons]
Name: "{group}\WRECKAGE"; Filename: "{app}\WRECKAGE.exe"
Name: "{group}\Uninstall WRECKAGE"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\WRECKAGE.exe"; Description: "Open WRECKAGE now"; Flags: nowait postinstall skipifsilent unchecked

[Messages]
FinishedLabel=WRECKAGE is installed. In FL Studio: Options > Manage plugins > Find more plugins, then look under Generators.%n%nHappy birthday, Jacob.
