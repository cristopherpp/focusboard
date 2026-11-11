param(
    [Parameter(Mandatory)][string]$StageDir,
    [Parameter(Mandatory)][string]$Version
)
$ErrorActionPreference = 'Stop'
$stage = (Resolve-Path $StageDir).Path
$required = @(
    'Focusboard.exe', 'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Qml.dll',
    'Qt6Quick.dll', 'Qt6QuickControls2.dll', 'Qt6Widgets.dll', 'qt.conf',
    'plugins/platforms/qwindows.dll', 'plugins/platforms/qoffscreen.dll',
    'qml/QtQuick/qmldir', 'qml/QtQuick/Controls/qmldir', 'qml/QtQuick/Controls/Fusion/qmldir',
    'msvcp140.dll', 'msvcp140_1.dll', 'msvcp140_2.dll', 'msvcp140_atomic_wait.dll',
    'vcruntime140.dll', 'vcruntime140_1.dll', 'LICENSE'
)
foreach ($file in $required) {
    if (!(Test-Path (Join-Path $stage $file) -PathType Leaf)) {
        throw "Missing deployment dependency: $file"
    }
}
# The executable and Qt libraries live at Prefix=.; Qt's default relative
# plugin and QML directories must therefore resolve beside the executable.
$qtConf = Get-Content (Join-Path $stage 'qt.conf')
if (!($qtConf -match '^\s*\[Paths\]\s*$') -or !($qtConf -match '^\s*Prefix\s*=\s*\.\s*$')) {
    throw 'qt.conf must set the deployment prefix to the executable directory'
}
$files = Get-ChildItem $stage -Recurse -File
$forbidden = $files | Where-Object {
    $_.Extension -in '.pdb', '.obj', '.lib', '.h', '.hpp' -or
    $_.Name -match '^Qt6.*d\.dll$|^vc_redist.*\.exe$|^focusboard-.*tests\.exe$'
}
if ($forbidden) { throw "Development/debug files in deployment: $($forbidden.FullName -join ', ')" }
$exe = Join-Path $stage 'Focusboard.exe'
if ((Get-Item $exe).VersionInfo.ProductVersion -ne $Version) {
    throw 'Executable version does not match CMake project version'
}
# Require an actual embedded icon group, not the Explorer default icon.
if (-not ('FocusboardResourceCheck' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class FocusboardResourceCheck {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr LoadLibraryEx(string file, IntPtr reserved, uint flags);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode)]
    public static extern IntPtr FindResource(IntPtr module, IntPtr name, IntPtr type);
    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr module);
}
'@
}
$module = [FocusboardResourceCheck]::LoadLibraryEx($exe, [IntPtr]::Zero, 2)
if ($module -eq [IntPtr]::Zero) { throw 'Cannot read executable resources' }
try {
    if ([FocusboardResourceCheck]::FindResource($module, [IntPtr]1, [IntPtr]14) -eq [IntPtr]::Zero) {
        throw 'Executable has no embedded Focusboard icon'
    }
} finally { [void][FocusboardResourceCheck]::FreeLibrary($module) }

# Restrict the child's DLL and QML search paths to the staged tree and Windows.
# The hosted runner still has system runtimes: this is not a clean-machine test.
$start = [Diagnostics.ProcessStartInfo]::new($exe)
$start.ArgumentList.Add('--verify-qml-resource')
$start.UseShellExecute = $false
$start.WorkingDirectory = $stage
foreach ($name in @('QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'QML_IMPORT_PATH',
                    'QML2_IMPORT_PATH', 'QTDIR', 'QT_WAYLAND_SHELL_INTEGRATION')) {
    [void]$start.Environment.Remove($name)
}
$start.Environment['PATH'] = "$stage;$env:SystemRoot\System32;$env:SystemRoot"
$start.Environment['QT_QPA_PLATFORM'] = 'offscreen'
$start.Environment['QT_QUICK_BACKEND'] = 'software'
$process = [Diagnostics.Process]::Start($start)
try {
    if (!$process.WaitForExit(60000)) {
        $process.Kill()
        throw 'Staged QML verification timed out'
    }
    if ($process.ExitCode -ne 0) { throw "Staged QML verification failed: $($process.ExitCode)" }
} finally { $process.Dispose() }
Write-Host "Verified Focusboard $Version staged runtime and resources."
