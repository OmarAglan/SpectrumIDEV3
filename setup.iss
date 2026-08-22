; مثبت قلم المستقل لويندوز.
; يضم واجهة قلم وQt وBaa-LSP الداخلي فقط. لا يضم أدوات سطر الأوامر ولا يعدل PATH.

#define MyAppId "{{1A6F6714-2C14-4DBD-BACB-B26CBABE36EE}"
#define MyAppName "قلم"
#ifndef MyAppVersion
  #define MyAppVersion "3.3.0"
#endif
#ifndef QalamPayloadDir
  #define QalamPayloadDir "dist\installer-payload"
#endif
#define MyAppPublisher "Omar Aglan"
#define MyAppURL "https://github.com/OmarAglan/Qalam-IDE"
#define MyAppExeName "Qalam.exe"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\Qalam
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
AllowNoIcons=yes
OutputDir=dist\installer
OutputBaseFilename=qalam-setup-{#MyAppVersion}-x64
#ifdef InstallerSignTool
SignTool={#InstallerSignTool}
SignedUninstaller=yes
#endif
SetupIconFile=qalam\resources\QalamLogo.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog commandline
SetupLogging=yes
UsePreviousAppDir=yes
UsePreviousLanguage=yes
UsePreviousTasks=yes
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName} {#MyAppVersion}

[Languages]
Name: "arabic"; MessagesFile: "compiler:Languages\Arabic.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "إنشاء اختصار على سطح المكتب"; GroupDescription: "اختصارات إضافية:"; Flags: unchecked

[Files]
Source: "{#QalamPayloadDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "README.md"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "documents\USER_GUIDE.md"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "documents\deployment.md"; DestDir: "{app}\docs"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\قلم\قلم"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autoprograms}\قلم\دليل المستخدم"; Filename: "{app}\docs\USER_GUIDE.md"
Name: "{autoprograms}\قلم\إزالة قلم"; Filename: "{uninstallexe}"
Name: "{autodesktop}\قلم"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "تشغيل قلم"; Flags: nowait postinstall skipifsilent unchecked

[Code]
const
  QALAM_INSTALLER_KEY = 'Software\BaaEcosystem\Qalam';

procedure QalamRegistryRoot(var Root: Integer);
begin
  if IsAdminInstallMode then
    Root := HKLM
  else
    Root := HKCU;
end;

function RunQalamHealthProbe: Boolean;
var
  ExitCode: Integer;
begin
  Result :=
    FileExists(ExpandConstant('{app}\{#MyAppExeName}')) and
    FileExists(ExpandConstant('{app}\Qt6Core.dll')) and
    FileExists(ExpandConstant('{app}\Qt6Gui.dll')) and
    FileExists(ExpandConstant('{app}\Qt6Widgets.dll')) and
    FileExists(ExpandConstant('{app}\platforms\qwindows.dll')) and
    FileExists(ExpandConstant('{app}\baa-lsp\baa-lsp.exe')) and
    Exec(ExpandConstant('{app}\baa-lsp\baa-lsp.exe'), '--version',
      ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, ExitCode) and
    (ExitCode = 0);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Root: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    if not RunQalamHealthProbe then
      RaiseException('فشل فحص قلم أو خادم Baa-LSP الداخلي بعد التثبيت.');
    QalamRegistryRoot(Root);
    RegWriteStringValue(Root, QALAM_INSTALLER_KEY, 'InstallLocation',
      ExpandConstant('{app}'));
    RegWriteStringValue(Root, QALAM_INSTALLER_KEY, 'Version', '{#MyAppVersion}');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Root: Integer;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    QalamRegistryRoot(Root);
    RegDeleteKeyIncludingSubkeys(Root, QALAM_INSTALLER_KEY);
  end;
end;
