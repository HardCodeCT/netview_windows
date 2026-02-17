; NetView Network Monitor - Inno Setup Script
; Clean production version 1.0.1

#define MyAppName "NetView"
#define MyAppVersion "1.0.1"
#define MyAppPublisher "NetView Software"
#define MyAppExeName "NetView.exe"

[Setup]
AppId={{8F9A3B2C-5E1D-4A7F-B3C6-9D8E4A2F1B5C}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}

DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

OutputDir=output
OutputBaseFilename=NetView_Setup_v{#MyAppVersion}

Compression=lzma
SolidCompression=yes
WizardStyle=modern

UninstallDisplayIcon={app}\{#MyAppExeName}

PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "startup"; Description: "Start {#MyAppName} automatically when Windows starts"; GroupDescription: "Startup Options:"; Flags: unchecked

[Files]
; Use absolute path - CHANGE THIS TO YOUR ACTUAL PATH
Source: "C:\Users\algorithm\Videos\netview\build\bin\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\algorithm\Videos\netview\docs\README.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme; Check: FileExists('C:\Users\algorithm\Videos\netview\docs\README.txt')
Source: "C:\Users\algorithm\Videos\netview\docs\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion; Check: FileExists('C:\Users\algorithm\Videos\netview\docs\LICENSE.txt')

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Registry]
; Startup registry entry (only if checkbox selected)
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; \
ValueType: string; ValueName: "NetView"; \
ValueData: """{app}\{#MyAppExeName}"""; Tasks: startup

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{cmd}"; Parameters: "/C taskkill /F /IM {#MyAppExeName}"; Flags: runhidden

[UninstallDelete]
Type: filesandordirs; Name: "{userappdata}\NetView"

[Code]

var
  AcceptCheckBox: TNewCheckBox;
  InfoMemo: TNewMemo;

procedure AcceptCheckBoxClick(Sender: TObject);
begin
  WizardForm.NextButton.Enabled := AcceptCheckBox.Checked;
end;

procedure InitializeWizard();
var
  InfoPage: TWizardPage;
begin
  InfoPage := CreateCustomPage(wpLicense, 'Important Information',
    'Please read the following information before continuing.');

  InfoMemo := TNewMemo.Create(InfoPage);
  InfoMemo.Parent := InfoPage.Surface;
  InfoMemo.Left := 0;
  InfoMemo.Top := 0;
  InfoMemo.Width := InfoPage.SurfaceWidth;
  InfoMemo.Height := InfoPage.SurfaceHeight - ScaleY(40);
  InfoMemo.ScrollBars := ssVertical;
  InfoMemo.ReadOnly := True;
  InfoMemo.Text :=
    'NETVIEW NETWORK MONITOR' + #13#10 + #13#10 +
    'Thank you for choosing NetView!' + #13#10 + #13#10 +
    'FEATURES:' + #13#10 +
    '  • Real-time network traffic monitoring' + #13#10 +
    '  • Process-level network tracking' + #13#10 +
    '  • Block processes from network access' + #13#10 +
    '  • Persistent blocking rules' + #13#10 + #13#10 +
    'SYSTEM REQUIREMENTS:' + #13#10 +
    '  • Windows 10/11 (64-bit)' + #13#10 +
    '  • Administrator privileges for installation' + #13#10 + #13#10 +
    'PRIVACY:' + #13#10 +
    '  • No data is transmitted externally' + #13#10 +
    '  • All monitoring is local only' + #13#10 + #13#10 +
    'By continuing, you agree to the license terms.';

  AcceptCheckBox := TNewCheckBox.Create(InfoPage);
  AcceptCheckBox.Parent := InfoPage.Surface;
  AcceptCheckBox.Left := 0;
  AcceptCheckBox.Top := InfoPage.SurfaceHeight - ScaleY(25);
  AcceptCheckBox.Width := InfoPage.SurfaceWidth;
  AcceptCheckBox.Caption := 'I have read and accept the terms';
  AcceptCheckBox.Checked := False;
  AcceptCheckBox.OnClick := @AcceptCheckBoxClick;

  WizardForm.NextButton.Enabled := False;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;

  if CurPageID = wpLicense + 1 then
  begin
    if not AcceptCheckBox.Checked then
    begin
      MsgBox('You must accept the terms before continuing.', mbError, MB_OK);
      Result := False;
    end;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\NetView');
  end;
end;