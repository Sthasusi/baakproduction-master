$ErrorActionPreference = 'Stop'
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    $installerPath = $PSCommandPath
    $child = Start-Process -FilePath 'powershell.exe' -Verb RunAs -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',('"' + $installerPath + '"')) -PassThru -Wait
    exit $child.ExitCode
}
try {
    $bundleName = 'BAD-I Master Pro.vst3'
    $sourcePath = Join-Path $PSScriptRoot $bundleName
    $binaryPath = Join-Path $sourcePath 'Contents\x86_64-win\BAD-I Master Pro.vst3'
    if (-not (Test-Path -LiteralPath $binaryPath -PathType Leaf)) { throw 'Extract the complete ZIP before running the installer.' }
    $manifestPath = Join-Path $PSScriptRoot 'BUILD.json'
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ((Get-FileHash -LiteralPath $binaryPath -Algorithm SHA256).Hash -ne $manifest.vst3_sha256) { throw 'The VST3 checksum does not match this package.' }
    $commonRoot = [IO.Path]::GetFullPath([Environment]::GetFolderPath('CommonProgramFiles'))
    $vstDirectory = [IO.Path]::GetFullPath((Join-Path $commonRoot 'VST3'))
    $destination = [IO.Path]::GetFullPath((Join-Path $vstDirectory $bundleName))
    if (-not $destination.StartsWith($vstDirectory + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid installation destination.' }
    New-Item -ItemType Directory -Path $vstDirectory -Force | Out-Null
    if (Test-Path -LiteralPath $destination) {
        $backupRoot = [IO.Path]::GetFullPath((Join-Path $vstDirectory 'BAD-I Backups'))
        $backupPath = [IO.Path]::GetFullPath((Join-Path $backupRoot ((Get-Date -Format 'yyyyMMdd-HHmmss') + '-BAD-I Master Pro.vst3.backup')))
        if (-not $backupPath.StartsWith($vstDirectory + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid backup destination.' }
        New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
        Move-Item -LiteralPath $destination -Destination $backupPath
        Write-Host "Previous version backed up to $backupPath"
    }
    Copy-Item -LiteralPath $sourcePath -Destination $destination -Recurse
    Write-Host "Installed: $destination"
    Write-Host 'Open your DAW and rescan VST3 plug-ins. No other plug-ins were changed.'
    Read-Host 'Press Enter to close'
} catch {
    Write-Error $_
    Read-Host 'Installation failed. Press Enter to close'
    exit 1
}
