# Run only on a disposable Windows account/CI runner: installs the real AppId.
param(
    [Parameter(Mandatory)][string]$SetupExe,
    [Parameter(Mandatory)][string]$Version,
    [Parameter(Mandatory)][string]$MeasurementsFile
)
$ErrorActionPreference = 'Stop'
$uninstallKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{DD73D447-98A0-4C30-9CE3-A940A0BB43E2}_is1'
if (Test-Path $uninstallKey) { throw 'Refusing to replace an existing Focusboard installation in an installer test' }
$temporary = Join-Path ([IO.Path]::GetTempPath()) ("Focusboard installer " + [guid]::NewGuid())
$installed = Join-Path $temporary 'App'
New-Item -ItemType Directory $temporary | Out-Null
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$before = (Get-ItemProperty $runKey -Name Focusboard -ErrorAction SilentlyContinue).Focusboard
try {
    $setup = Start-Process -FilePath (Resolve-Path $SetupExe) -ArgumentList @(
        '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', "/DIR=`"$installed`"",
        "/LOG=`"$temporary/setup.log`""
    ) -PassThru -Wait
    if ($setup.ExitCode -ne 0) { throw "Installer failed: $($setup.ExitCode)" }
    $metadata = Get-ItemProperty $uninstallKey
    if ($metadata.DisplayVersion -ne $Version -or $metadata.Publisher -ne 'Focusboard contributors') {
        throw 'Installed metadata does not match the release'
    }
    $after = (Get-ItemProperty $runKey -Name Focusboard -ErrorAction SilentlyContinue).Focusboard
    if ($before -ne $after) { throw 'Installation changed autostart without consent' }
    if (Get-Process Focusboard -ErrorAction SilentlyContinue) { throw 'Installer launched Focusboard' }
    & "$PSScriptRoot/verify-windows-stage.ps1" -StageDir $installed -Version $Version
    $measurements = Get-Content $MeasurementsFile -Raw | ConvertFrom-Json
    $installedBytes = (Get-ChildItem $installed -File -Recurse | Measure-Object Length -Sum).Sum
    $measurements | Add-Member -NotePropertyName installedFileBytes -NotePropertyValue $installedBytes -Force
    $measurements | ConvertTo-Json | Set-Content $MeasurementsFile

    $uninstall = Start-Process -FilePath "$installed/unins000.exe" -ArgumentList @(
        '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'
    ) -PassThru -Wait
    if ($uninstall.ExitCode -ne 0) { throw "Uninstaller failed: $($uninstall.ExitCode)" }
    if (Test-Path "$installed/Focusboard.exe") { throw 'Uninstaller left the executable behind' }
    if (Test-Path $uninstallKey) { throw 'Uninstaller left its registration behind' }
    $after = (Get-ItemProperty $runKey -Name Focusboard -ErrorAction SilentlyContinue).Focusboard
    if ($before -ne $after) { throw 'Uninstaller modified an unrelated autostart registration' }
    if ($env:GITHUB_STEP_SUMMARY) {
        "Installed file bytes (including uninstaller, excluding user data): $installedBytes" |
            Add-Content $env:GITHUB_STEP_SUMMARY
    }
} finally {
    # Only remove our generated temporary tree. Keep setup logs in failures.
    if (!(Test-Path $uninstallKey)) { Remove-Item $temporary -Recurse -Force }
}
