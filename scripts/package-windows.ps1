param(
    [string]$BuildDir = 'build-windows',
    [string]$StageDir = 'stage-windows',
    [string]$OutputDir = 'release-artifacts',
    [string]$Iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$version = (Get-Content (Join-Path $BuildDir 'version.txt') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'Invalid CMake project version' }
if ($env:GITHUB_REF -like 'refs/tags/*' -and $env:GITHUB_REF_NAME -ne "v$version") {
    throw "Tag $env:GITHUB_REF_NAME does not match CMake version $version"
}
# Avoid packaging obsolete DLLs from a previous deployment.
if (Test-Path $StageDir) { throw "Use a fresh staging directory: $StageDir" }
& cmake --install $BuildDir --config Release --prefix ([IO.Path]::GetFullPath($StageDir))
if ($LASTEXITCODE -ne 0) { throw 'CMake runtime deployment failed' }
& "$PSScriptRoot/verify-windows-stage.ps1" -StageDir $StageDir -Version $version
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$stage = (Resolve-Path $StageDir).Path
$output = (Resolve-Path $OutputDir).Path
$zip = Join-Path $output 'Focusboard-Portable-x64.zip'
Compress-Archive -Path "$stage/*" -DestinationPath $zip -CompressionLevel Optimal -Force

# Verify the archive itself, including extraction into a path containing spaces.
$extracted = Join-Path ([IO.Path]::GetTempPath()) ("Focusboard portable 日本 " + [guid]::NewGuid())
try {
    Expand-Archive $zip $extracted
    & "$PSScriptRoot/verify-windows-stage.ps1" -StageDir $extracted -Version $version
} finally {
    if (Test-Path $extracted) { Remove-Item $extracted -Recurse -Force }
}
& $Iscc "/DAppVersion=$version" "/DStageDir=$stage" "/DOutputDir=$output" "$root/packaging/windows/focusboard.iss"
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed' }
$setup = Join-Path $output 'Focusboard-Setup-x64.exe'
if (!(Test-Path $setup)) { throw 'Installer was not produced' }
$measurements = [ordered]@{
    version = $version
    installerBytes = (Get-Item $setup).Length
    portableZipBytes = (Get-Item $zip).Length
    stagedFileBytes = (Get-ChildItem $stage -File -Recurse | Measure-Object Length -Sum).Sum
    stagedFileCount = (Get-ChildItem $stage -File -Recurse | Measure-Object).Count
    note = 'File bytes, not allocated disk clusters. Staged bytes exclude the uninstaller; installed bytes, when present, include it. All exclude user data. Startup, idle CPU and memory not measured.'
}
$measurements | ConvertTo-Json | Set-Content (Join-Path $output "Focusboard-$version-measurements.json")
$hashes = foreach ($file in @($setup, $zip)) {
    $hash = Get-FileHash $file -Algorithm SHA256
    "$($hash.Hash.ToLower())  $([IO.Path]::GetFileName($file))"
}
$hashes | Set-Content (Join-Path $output "Focusboard-$version-SHA256SUMS.txt")
if ($env:GITHUB_STEP_SUMMARY) {
    "Focusboard $version package measurements (bytes):" | Add-Content $env:GITHUB_STEP_SUMMARY
    $measurements | ConvertTo-Json | Add-Content $env:GITHUB_STEP_SUMMARY
}
