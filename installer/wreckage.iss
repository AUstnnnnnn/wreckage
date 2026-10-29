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
MinVersion=10.0
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
Source: "deps\MicrosoftEdgeWebview2Setup.exe"; Flags: dontcopy
; The VST3 goes where FL Studio and every other Windows host looks.
Source: "{#Src}\VST3\WRECKAGE.vst3\*"; DestDir: "{commoncf64}\VST3\WRECKAGE.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Src}\Standalone\WRECKAGE.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\INSTALL.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\WRECKAGE"; Filename: "{app}\WRECKAGE.exe"
Name: "{group}\Uninstall WRECKAGE"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\WRECKAGE.exe"; Description: "Open WRECKAGE now"; Flags: nowait postinstall skipifsilent unchecked

[Messages]
FinishedLabel=WRECKAGE is installed. In FL Studio: Options > Manage plugins > Find more plugins, then look under Generators.%n%nHappy birthday, Jacob.

[Code]
function HasRuntimeVersion(Root: Integer): Boolean;
var
  Version: String;
begin
  Result := RegQueryStringValue(Root,
    'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}',
    'pv', Version) and (Version <> '') and (Version <> '0.0.0.0');
end;

function WebView2Installed: Boolean;
begin
  Result := HasRuntimeVersion(HKLM32) or HasRuntimeVersion(HKCU32) or
    HasRuntimeVersion(HKCU64);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Result := '';
  if WebView2Installed then Exit;
  ExtractTemporaryFile('MicrosoftEdgeWebview2Setup.exe');
  if not Exec(ExpandConstant('{tmp}\MicrosoftEdgeWebview2Setup.exe'),
    '/silent /install', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    Result := 'Could not start Microsoft WebView2 setup. Install the Evergreen Runtime from https://developer.microsoft.com/microsoft-edge/webview2/ and retry.';
    Exit;
  end;
  if not WebView2Installed then
    Result := 'Microsoft WebView2 is required for the interface. Connect to the internet and retry, or install the Evergreen Runtime from https://developer.microsoft.com/microsoft-edge/webview2/. Setup exit code: ' + IntToStr(ResultCode);
end;
