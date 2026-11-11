[CmdletBinding()]
param([switch]$Update, [switch]$Uninstall, [switch]$Status)
$ErrorActionPreference = 'Stop'
if (@(@($Update,$Uninstall,$Status) | Where-Object { $_ }).Count -gt 1) { throw 'Choose only one of -Update, -Uninstall, or -Status.' }
function Fail([string]$message) { throw "Focusboard: $message" }
if (@(@($Update,$Uninstall,$Status) | Where-Object { $_ }).Count -gt 1) { Fail 'Choose only one mode.' }
if ($env:OS -ne 'Windows_NT' -or -not [Environment]::Is64BitOperatingSystem) { Fail 'Windows 10/11 x64 is required.' }
$os = Get-CimInstance Win32_OperatingSystem
if ([version]$os.Version -lt [version]'10.0.17763') { Fail 'Windows 10 build 17763 or later is required.' }
$uninstallKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{DD73D447-98A0-4C30-9CE3-A940A0BB43E2}_is1'
$installed = if (Test-Path $uninstallKey) { Get-ItemProperty $uninstallKey } else { $null }
if ($Uninstall) {
    if (!$installed) { Write-Host 'Focusboard is not installed through Inno Setup.'; exit 0 }
    $uninstaller = $installed.UninstallString.Trim('"')
    if (!(Test-Path $uninstaller -PathType Leaf) -or [IO.Path]::GetFileName($uninstaller) -notmatch '^unins[0-9]+\.exe$') { Fail 'The registered Focusboard uninstaller is missing or invalid. Use Windows Settings > Installed apps.' }
    $running = Get-Process Focusboard -ErrorAction SilentlyContinue
    if ($running) { Fail 'Quit Focusboard from its tray menu before uninstalling.' }
    $process = Start-Process -FilePath $uninstaller -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -PassThru -Wait
    if ($process.ExitCode -ne 0) { Fail "Uninstaller exited with code $($process.ExitCode)." }
    Write-Host 'Focusboard removed. Your Markdown and settings remain.'
    exit 0
}
$root = Split-Path $PSScriptRoot -Parent
$git = Get-Command git -ErrorAction SilentlyContinue
if (!$git) { Fail 'Git is required. Install Git for Windows and reopen PowerShell.' }
$origin = (& git -C $root remote get-url origin).Trim()
if ($LASTEXITCODE -ne 0 -or $origin -notmatch '^https://github\.com/cristopherpp/focusboard(?:\.git)?/?$') { Fail 'The origin must be https://github.com/cristopherpp/focusboard.git. Check git remote -v.' }
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('Focusboard prebuilt 日本 ' + [guid]::NewGuid().ToString('N'))
try {
    & git clone --quiet --depth 1 --branch prebuilt --single-branch -- $origin $temporary
    if ($LASTEXITCODE -ne 0) { Fail 'The prebuilt branch is not published yet. Wait for a successful main build and retry.' }
    $manifestPath = Join-Path $temporary 'latest.json'
    if (!(Test-Path $manifestPath -PathType Leaf)) { Fail 'The prebuilt branch has no latest.json.' }
    $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
    $keys = @($manifest.PSObject.Properties.Name | Sort-Object)
    if (($keys -join ',') -ne 'architecture,installer,platform,portable,schemaVersion,sourceCommit,version' -or
        ($manifest.schemaVersion -isnot [int] -and $manifest.schemaVersion -isnot [long]) -or $manifest.schemaVersion -ne 1 -or
        $manifest.platform -cne 'windows' -or $manifest.architecture -cne 'x64' -or
        $manifest.version -cnotmatch '^\d+\.\d+\.\d+$' -or
        $manifest.sourceCommit -cnotmatch '^[0-9a-f]{40}$') { Fail 'Invalid prebuilt manifest.' }
    $packages = @{ 'Focusboard-Setup-x64.exe' = $manifest.installer; 'Focusboard-Portable-x64.zip' = $manifest.portable }
    foreach ($filename in $packages.Keys) {
        $item=$packages[$filename]
        if (!$item -or (($item.PSObject.Properties.Name | Sort-Object) -join ',') -cne 'filename,sha256,size' -or
            $item.filename -cne $filename -or $item.sha256 -cnotmatch '^[0-9a-f]{64}$' -or
            $item.size -isnot [long] -and $item.size -isnot [int] -or $item.size -le 0 -or $item.size -ge 100000000) { Fail "Invalid package metadata: $filename" }
        $path=Join-Path $temporary $filename
        if (!(Test-Path $path -PathType Leaf)) { Fail "Missing package: $filename" }
        if ((Get-Item $path).Length -ne $item.size -or (Get-FileHash $path -Algorithm SHA256).Hash.ToLowerInvariant() -cne $item.sha256) { Fail "Package size or SHA-256 mismatch: $filename" }
    }
    if ($Status) {
        $current=if ($installed) { $installed.DisplayVersion } else { 'not installed' }
        Write-Host "Installed: $current; available: $($manifest.version); source commit: $($manifest.sourceCommit)"
        exit 0
    }
    if ($installed -and $installed.DisplayVersion -eq $manifest.version) { Write-Host "Focusboard $($manifest.version) is already installed."; exit 0 }
    if (Get-Process Focusboard -ErrorAction SilentlyContinue) { Fail 'Quit Focusboard from its tray menu before installing or updating.' }
    $setup=Join-Path $temporary 'Focusboard-Setup-x64.exe'
    $process=Start-Process -FilePath $setup -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/NOCLOSEAPPLICATIONS' -PassThru -Wait
    if ($process.ExitCode -ne 0) { Fail "Installer exited with code $($process.ExitCode). The previous installation was not intentionally removed." }
    $result=Get-ItemProperty $uninstallKey -ErrorAction Stop
    if ($result.DisplayVersion -ne $manifest.version) { Fail 'Installer completed but registered version does not match the verified package.' }
    Write-Host "Focusboard $($manifest.version) installed for the current user. Open it from Start."
} finally {
    if (Test-Path $temporary) { Remove-Item $temporary -Recurse -Force }
}
