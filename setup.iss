; Written by ChatGPT, idk Inno Setup syntax fr
#define MyAppName "LifeSaver"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "KAS Software"
#define MyAppExeName "LifeSaver.exe"

[Setup]
AppId={{97C7431D-5369-4DC1-BA1B-72296347DA2B}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\LifeSaver
DefaultGroupName=LifeSaver

; Per-user installation only. No elevation for installer.
PrivilegesRequired=lowest

; License agreement
LicenseFile=LICENSE.txt

; Installer output
OutputDir=.builds\installer
OutputBaseFilename=LifeSaver-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=assets\setup.ico

; Uninstaller
UninstallDisplayName=LifeSaver
UninstallDisplayIcon={app}\LifeSaver.exe

; Avoid architecture restrictions
ArchitecturesInstallIn64BitMode=x64compatible

[Tasks]
Name: "startup"; \
    Description: "Launch LifeSaver automatically when I sign in"; \
    Flags: checkedonce

Name: "startup\admin"; \
    Description: "Run LifeSaver as Administrator"; \
    Check: WizardIsTaskSelected('startup')

[Files]
Source: ".builds\release\main.exe"; \
    DestDir: "{app}"; \
    DestName: "{#MyAppExeName}"; \
    Flags: ignoreversion

[Icons]
; Start Menu shortcut
Name: "{autoprograms}\LifeSaver"; \
    Filename: "{app}\{#MyAppExeName}"

; Startup shortcut
Name: "{userstartup}\LifeSaver"; \
    Filename: "{app}\{#MyAppExeName}"; \
    Tasks: startup

; Uninstaller shortcut
Name: "{autoprograms}\LifeSaver\Uninstall LifeSaver"; \
    Filename: "{uninstallexe}"

[Registry]
; Windows compatibility setting for elevated execution.
; Applies RUNASADMIN to LifeSaver.exe for this user.
Root: HKCU; \
    Subkey: "Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; \
    ValueType: string; \
    ValueName: "{app}\{#MyAppExeName}"; \
    ValueData: "~ RUNASADMIN"; \
    Flags: uninsdeletevalue; \
    Tasks: startup\admin

[UninstallDelete]
; Remove the Startup shortcut if it exists
Type: files; Name: "{userstartup}\LifeSaver.lnk"
