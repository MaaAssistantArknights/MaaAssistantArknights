; Inno Setup Script
; Non-commercial use only

#include "maa_packageinfo.iss"


[Setup]
AppId="{#MaaPublisherIdentifier}.{#MaaAppIdentifier}"
AppName="{#MaaAppName}"
AppVersion="{#MaaAppVersion}"
AppVerName="{#MaaAppName} {#MaaAppVersion}"
AppPublisher="{#MaaPublisherName}"
AppPublisherURL="https://github.com/MaaAssistantArknights"
AppSupportURL="https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues"
AppUpdatesURL="https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases"
DefaultDirName="{userpf}\{#MaaAppName}"
UninstallDisplayIcon="{app}\MAA.exe"
ArchitecturesAllowed="{#Architecture}"
ArchitecturesInstallIn64BitMode="{#Architecture}"
DefaultGroupName="{#MaaAppName}"
AllowNoIcons="yes"
AlwaysShowDirOnReadyPage="yes"
AlwaysShowGroupOnReadyPage="yes"
LicenseFile="{#MaaLicensePath}"
PrivilegesRequired="lowest"
SetupIconFile="{#MaaIconPath}"
SolidCompression="yes"
OutputDir="{#MaaOutputDir}"
OutputBaseFilename="{#MaaOutputBaseFilename}"
WizardStyle="modern"
; 6.6.0 and later
; WizardStyle="modern dynamic"

[Languages]
Name: "zh_cn"; MessagesFile: "Languages\ChineseSimplified.isl"
Name: "zh_tw"; MessagesFile: "Languages\ChineseTraditional.isl"
Name: "en_us"; MessagesFile: "compiler:Default.isl"
Name: "ja_jp"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "ko_kr"; MessagesFile: "compiler:Languages\Korean.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#MaaPackagePath}\MAA.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MaaPackagePath}\*"; Excludes: "\*.zip"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MaaAppName}"; Filename: "{app}\MAA.exe"
Name: "{group}\{cm:UninstallProgram,{#MaaAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MaaAppName}"; Filename: "{app}\MAA.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\MAA.exe"; Description: "{cm:LaunchProgram,{#MaaAppName}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\RemoveAutoStartReg_移除开机自启注册表项.bat"; RunOnceId: "RemoveAutoStartReg"; Flags: runhidden skipifdoesntexist

[UninstallDelete]
Type: filesandordirs; Name: "{app}\cache"
Type: filesandordirs; Name: "{app}\debug"
Type: filesandordirs; Name: "{app}\config"
Type: filesandordirs; Name: "{app}\data"
