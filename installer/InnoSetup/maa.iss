; For Inno Setup
; Non-commercial use only

#include "maa_packageinfo.iss"


[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
AppId="{#MaaPublisherIdentifier}.{#MaaAppIdentifier}"
AppName="{#MaaAppName}"
AppVersion="{#MyAppVersion}"
AppVerName="{#MaaAppName} {#MyAppVersion}"
AppPublisher="{#MaaPublisherName}"
AppPublisherURL="https://github.com/MaaAssistantArknights"
AppSupportURL="https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues"
AppUpdatesURL="https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases"
DefaultDirName="{userpf}\{#MaaAppName}"
UninstallDisplayIcon="{app}\{MAA.exe}"
; "ArchitecturesAllowed=x64compatible" specifies that Setup cannot run
; on anything but x64 and Windows 11 on Arm.
ArchitecturesAllowed="{#Architecture}"
; "ArchitecturesInstallIn64BitMode=x64compatible" requests that the
; install be done in "64-bit mode" on x64 or Windows 11 on Arm,
; meaning it should use the native 64-bit Program Files directory and
; the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode="{#Architecture}"
DefaultGroupName="{#MaaAppName}"
AllowNoIcons="yes"
AlwaysShowDirOnReadyPage="yes"
AlwaysShowGroupOnReadyPage="yes"
LicenseFile="{#LicenseFilePath}"
; Remove the following line to run in administrative install mode (install for all users).
PrivilegesRequired="lowest"
OutputBaseFilename="MAA-v{#MyAppVersion}-win-{#ArchitectureRaw}-Installer"
SetupIconFile="{#IconFilePath}"
SolidCompression="yes"
WizardStyle="modern dynamic"

[Languages]
Name: "zh_cn"; MessagesFile: "{#UnofficialLanguageFolderPath}\ChineseSimplified.isl"
Name: "zh_tw"; MessagesFile: "{#UnofficialLanguageFolderPath}\ChineseTraditional.isl"
Name: "en_us"; MessagesFile: "compiler:Default.isl"
Name: "ja_jp"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "ko_kr"; MessagesFile: "compiler:Languages\Korean.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#PackageFolderPath}\{MAA.exe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PackageFolderPath}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{#MaaAppName}"; Filename: "{app}\{MAA.exe}"
Name: "{group}\{cm:UninstallProgram,{#MaaAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MaaAppName}"; Filename: "{app}\{MAA.exe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{MAA.exe}"; Description: "{cm:LaunchProgram,{#MaaAppName}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\RemoveAutoStartReg_移除开机自启注册表项.bat"; RunOnceId: "RemoveAutoStartReg"; Flags: runhidden skipifdoesntexist

[UninstallDelete]
Type: filesandordirs; Name: "{app}\cache"
Type: filesandordirs; Name: "{app}\debug"
Type: filesandordirs; Name: "{app}\config"
Type: filesandordirs; Name: "{app}\data"
