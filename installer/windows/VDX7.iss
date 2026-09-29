#define AppName "VDX7 Mk1."
#define AppVersion "1.0.0"
#ifndef PluginBundle
  #error "Pass /DPluginBundle=<absolute path to VDX7.vst3>"
#endif

[Setup]
AppId={{9F5E28E9-1D9A-4B25-99CA-1F0B08C6C087}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=RobCZart82
DefaultDirName={commoncf64}\VST3\VDX7.vst3
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
Uninstallable=yes
OutputBaseFilename=VDX7-1.0.0-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no

[Files]
Source: "{#PluginBundle}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

