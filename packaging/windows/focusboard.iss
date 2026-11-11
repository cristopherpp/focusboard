; Compile the exact cmake --install tree; no SDK or build-tree files.
#ifndef AppVersion
  #error AppVersion must be supplied from build/version.txt
#endif
#ifndef StageDir
  #error StageDir must name the CMake installation directory
#endif
#ifndef OutputDir
  #error OutputDir must name the artifact directory
#endif

[Setup]
AppId={{DD73D447-98A0-4C30-9CE3-A940A0BB43E2}
AppName=Focusboard
AppVersion={#AppVersion}
AppPublisher=Focusboard contributors
AppPublisherURL=https://github.com/cristopherpp/focusboard
AppSupportURL=https://github.com/cristopherpp/focusboard/issues
AppUpdatesURL=https://github.com/cristopherpp/focusboard/releases
DefaultDirName={localappdata}\Programs\Focusboard
DefaultGroupName=Focusboard
PrivilegesRequired=lowest
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0.17763
OutputDir={#OutputDir}
OutputBaseFilename=Focusboard-Setup-x64
SetupIconFile=..\..\assets\icons\focusboard.ico
UninstallDisplayIcon={app}\Focusboard.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
DisableProgramGroupPage=yes
DisableWelcomePage=yes
CloseApplications=yes
RestartApplications=no
LicenseFile=..\..\LICENSE

[Tasks]
Name: desktopicon; Description: "Create a desktop shortcut"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{userprograms}\Focusboard"; Filename: "{app}\Focusboard.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\Focusboard"; Filename: "{app}\Focusboard.exe"; WorkingDir: "{app}"; Tasks: desktopicon

; No [Run] or autostart installation entry: first launch is the user's choice.
; Remove only an autostart entry pointing at this installation. Preserve settings,
; documents, and registrations belonging to a portable copy.
[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Command: String;
begin
  if CurUninstallStep = usUninstall then
    if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run',
                           'Focusboard', Command) then
      if CompareText(Command, '"' + ExpandConstant('{app}\Focusboard.exe') + '"') = 0 then
        RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'Focusboard');
end;
