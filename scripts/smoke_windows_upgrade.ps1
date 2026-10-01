param(
    [Parameter(Mandatory = $true)][string]$CandidateSetup,
    [Parameter(Mandatory = $true)][string]$PluginBundle,
    [Parameter(Mandatory = $true)][string]$Output
)

# This integration smoke deliberately refuses local or self-hosted machines.
# It runs old/new installers only on a disposable GitHub-hosted Windows runner.
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
if ($env:GITHUB_ACTIONS -ne "true" -or $env:RUNNER_OS -ne "Windows" -or
    $env:RUNNER_ENVIRONMENT -ne "github-hosted") {
    throw "Upgrade smoke requires a disposable GitHub-hosted Windows runner"
}

$candidate = Get-Item -LiteralPath $CandidateSetup
$bundle = Get-Item -LiteralPath $PluginBundle
if ($candidate.Name -ne "VDX7-1.0.1-Windows-x64-Setup.exe" -or
    -not $bundle.PSIsContainer -or $bundle.Name -ne "VDX7.vst3") {
    throw "Unexpected candidate installer or plugin bundle"
}
if (Test-Path -LiteralPath $Output) { throw "Upgrade evidence output already exists" }
$installDir = Join-Path $env:ProgramFiles "Common Files\VST3\VDX7.vst3"
$registryPath = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{9F5E28E9-1D9A-4B25-99CA-1F0B08C6C087}_is1"
$bankPath = Join-Path $env:APPDATA "VDX7-JUCE\User Banks\USER.vub"
$documents = [Environment]::GetFolderPath("MyDocuments")
$userMarker = Join-Path $documents "VDX7-JUCE\installer-upgrade-preservation.txt"
foreach ($path in @($installDir, $registryPath, $bankPath, $userMarker)) {
    if (Test-Path -LiteralPath $path) { throw "Upgrade smoke refuses existing installation or user fixtures" }
}

$testDir = Join-Path $env:RUNNER_TEMP ("vdx7-upgrade-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $testDir | Out-Null
$oldSetup = Join-Path $testDir "VDX7-1.0.0-Windows-x64-Setup.exe"
$oldDigest = "670b18160fd67c3534c5ca2914353762a730e324765189750434fd8e202206e3"
Invoke-WebRequest -Uri "https://github.com/RobCZart82/VDX7-JUCE/releases/download/v1.0.0/VDX7-1.0.0-Windows-x64-Setup.exe" -OutFile $oldSetup
if ((Get-FileHash -LiteralPath $oldSetup -Algorithm SHA256).Hash.ToLowerInvariant() -ne $oldDigest) {
    throw "Published 1.0.0 installer digest mismatch; will not execute it"
}

function Invoke-Installer([string]$Executable) {
    $result = Start-Process -FilePath $Executable -ArgumentList "/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/SP-" -Wait -PassThru
    if ($result.ExitCode -ne 0) { throw "Installer/uninstaller failed: $($result.ExitCode)" }
}

function Assert-RegisteredVersion([string]$Version) {
    $entry = Get-ItemProperty -LiteralPath $registryPath
    if ($entry.DisplayName -notmatch '^VDX7 Mk1\.(?:\s|$)' -or $entry.DisplayVersion -ne $Version) {
        throw "Stable AppId registration/version mismatch"
    }
    $registrations = @(Get-ChildItem "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall" |
        Get-ItemProperty | Where-Object { $_.PSObject.Properties["DisplayName"] -and $_.DisplayName -match '^VDX7 Mk1\.(?:\s|$)' })
    if ($registrations.Count -ne 1) { throw "Duplicate VDX7 uninstall registrations" }
}

Invoke-Installer $oldSetup
Assert-RegisteredVersion "1.0.0"
if (-not (Test-Path -LiteralPath (Join-Path $installDir "unins000.exe"))) {
    throw "Published installer layout differs from reviewed baseline"
}

# Synthetic byte-preservation markers, NOT valid bank/firmware/audio fixtures.
foreach ($path in @($bankPath, $userMarker)) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force | Out-Null
    [IO.File]::WriteAllText($path, "VDX7 CI installer preservation marker; not ROM or a playable bank")
}
$bankHash = (Get-FileHash -LiteralPath $bankPath -Algorithm SHA256).Hash
$markerHash = (Get-FileHash -LiteralPath $userMarker -Algorithm SHA256).Hash

function Assert-PreservedUserFiles {
    if ((Get-FileHash -LiteralPath $bankPath -Algorithm SHA256).Hash -ne $bankHash -or
        (Get-FileHash -LiteralPath $userMarker -Algorithm SHA256).Hash -ne $markerHash) {
        throw "Upgrade/uninstall changed user-owned file bytes"
    }
}

Invoke-Installer $candidate.FullName
Assert-RegisteredVersion "1.0.1"
Assert-PreservedUserFiles
$payloadFiles = @(Get-ChildItem -LiteralPath $bundle.FullName -Recurse -File)
if ($payloadFiles.Count -eq 0) { throw "Candidate bundle is empty" }
foreach ($file in $payloadFiles) {
    $relative = [IO.Path]::GetRelativePath($bundle.FullName, $file.FullName)
    $installedFile = Join-Path $installDir $relative
    if ((Get-FileHash -LiteralPath $installedFile -Algorithm SHA256).Hash -ne
        (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash) {
        throw "Upgraded plugin payload differs from candidate"
    }
}

$entry = Get-ItemProperty -LiteralPath $registryPath
$uninstallExe = $entry.UninstallString.Trim('"')
if ([IO.Path]::GetFullPath($uninstallExe) -ne [IO.Path]::GetFullPath((Join-Path $installDir "unins000.exe"))) {
    throw "Unexpected upgraded uninstaller path"
}
Invoke-Installer $uninstallExe
if ((Test-Path -LiteralPath $installDir) -or (Test-Path -LiteralPath $registryPath)) {
    throw "Upgraded uninstall left the bundle or AppId registration behind"
}
Assert-PreservedUserFiles

@(
    "PASS: published 1.0.0 -> candidate 1.0.1 Windows x64 installer upgrade/uninstall",
    "Baseline installer SHA256: $oldDigest",
    "Candidate installer SHA256: $((Get-FileHash -LiteralPath $candidate.FullName -Algorithm SHA256).Hash.ToLowerInvariant())",
    "Stable AppId and existing uninstall directory preserved; exactly one registration",
    "All $($payloadFiles.Count) candidate payload file hashes match installed files",
    "Upgraded uninstaller removed bundle/registration; synthetic USER/document bytes preserved",
    "Not a host/audio test, playable USER bank test, firmware test or private-machine acceptance"
) | Set-Content -LiteralPath $Output -Encoding utf8
Write-Output "PASS: Windows 1.0.0 -> 1.0.1 upgrade/uninstall and user-file preservation"
