[CmdletBinding()]
param(
    [ValidateSet('32.2.2', '33.0.1')]
    [string] $ObsVersion = '33.0.1'
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildProfile = switch ($ObsVersion) {
    '32.2.2' {
        @{
            Family = 'OBS32'
            Layout = 'legacy'
            DependenciesVersion = '2025-07-11'
            DependenciesHash = 'c8c642c1070dc31ce9a0f1e4cef5bb992f4bff4882255788b5da12129e85caa7'
            QtHash = '0e76bf0555dd5382838850b748d3dcfab44a1e1058441309ab54e1a65b156d0a'
            SourceHash = 'ec81fb66b03e75ddb3076b576f62679c39262e0e9960cef3e17a40dc5d68e6b4'
            MinimumWindowsSdk = [version]'10.0.20348.0'
        }
    }
    '33.0.1' {
        @{
            Family = 'OBS33'
            Layout = 'modern'
            DependenciesVersion = '2026-10-23'
            DependenciesHash = '7508972a867a0ef68c26cd8bdba61fd999edd0a92fa9eb9585774de9bf51bfbe'
            QtHash = '78cd7a138464bb6edcdd3b85fcd63a5a7c69e369629963f3bc395bab74fb3f47'
            SourceHash = 'c8d3a8f69a0f17970ac0a8f2a2462f1bc4dacd324f6da0c4b408d442d4798913'
            MinimumWindowsSdk = [version]'10.0.26100.0'
        }
    }
}

$projectVersionMatch = Select-String -Path (Join-Path $projectRoot 'CMakeLists.txt') `
    -Pattern '^project\(CountdownStudio VERSION ([0-9]+\.[0-9]+\.[0-9]+) LANGUAGES CXX\)$'
if (-not $projectVersionMatch) {
    throw 'Could not read Countdown Studio version from CMakeLists.txt.'
}
$projectVersion = $projectVersionMatch.Matches[0].Groups[1].Value

if ($env:GITHUB_REF_TYPE -eq 'tag') {
    $tagVersion = $env:GITHUB_REF_NAME -replace '^v', ''
    if ($tagVersion -ne $projectVersion) {
        throw "Release tag '$env:GITHUB_REF_NAME' does not match project version '$projectVersion'."
    }
}

$buildRoot = Join-Path $projectRoot "build-obs-$ObsVersion"
$dependenciesRoot = Join-Path $buildRoot 'dependencies'
$pluginBuild = Join-Path $buildRoot 'plugin'
$packageRoot = Join-Path $buildRoot 'packages'
New-Item -ItemType Directory -Path $dependenciesRoot, $packageRoot -Force | Out-Null

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory)] [string] $Command,
        [Parameter(Mandatory)] [string[]] $Arguments
    )

    Write-Host "> $Command $($Arguments -join ' ')"
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command '$Command' failed with exit code $LASTEXITCODE."
    }
}

function Get-VerifiedDownload {
    param(
        [Parameter(Mandatory)] [string] $Uri,
        [Parameter(Mandatory)] [string] $Path,
        [Parameter(Mandatory)] [string] $Sha256
    )

    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        $existingHash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($existingHash -eq $Sha256.ToLowerInvariant()) {
            return
        }
        throw "Cached dependency hash mismatch at '$Path'; remove that build cache before retrying."
    }

    $partialPath = "$Path.$([guid]::NewGuid().ToString('N')).partial"
    try {
        Invoke-WebRequest -Uri $Uri -OutFile $partialPath
        $downloadHash = (Get-FileHash -LiteralPath $partialPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($downloadHash -ne $Sha256.ToLowerInvariant()) {
            throw "SHA-256 mismatch for '$Uri'. Expected $Sha256, got $downloadHash."
        }
        Move-Item -LiteralPath $partialPath -Destination $Path
    }
    finally {
        if (Test-Path -LiteralPath $partialPath -PathType Leaf) {
            Remove-Item -LiteralPath $partialPath -Force
        }
    }
}

$obsDepsArchiveName = "windows-deps-$($buildProfile.DependenciesVersion)-x64.zip"
$qtArchiveName = "windows-deps-qt6-$($buildProfile.DependenciesVersion)-x64.zip"
$sourceArchiveName = "OBS-Studio-$ObsVersion-Sources.tar.gz"
$obsDepsArchive = Join-Path $dependenciesRoot $obsDepsArchiveName
$qtArchive = Join-Path $dependenciesRoot $qtArchiveName
$sourceArchive = Join-Path $dependenciesRoot $sourceArchiveName
$releaseBase = "https://github.com/obsproject/obs-deps/releases/download/$($buildProfile.DependenciesVersion)"
$sourceUrl = "https://github.com/obsproject/obs-studio/releases/download/$ObsVersion/$sourceArchiveName"

Get-VerifiedDownload -Uri "$releaseBase/$obsDepsArchiveName" -Path $obsDepsArchive -Sha256 $buildProfile.DependenciesHash
Get-VerifiedDownload -Uri "$releaseBase/$qtArchiveName" -Path $qtArchive -Sha256 $buildProfile.QtHash
Get-VerifiedDownload -Uri $sourceUrl -Path $sourceArchive -Sha256 $buildProfile.SourceHash

$obsDepsRoot = Join-Path $dependenciesRoot "obs-deps-$($buildProfile.DependenciesVersion)-x64"
$qtRoot = Join-Path $dependenciesRoot "obs-deps-qt6-$($buildProfile.DependenciesVersion)-x64"
if (-not (Test-Path -LiteralPath (Join-Path $obsDepsRoot 'share\obs-deps\VERSION'))) {
    Expand-Archive -LiteralPath $obsDepsArchive -DestinationPath $obsDepsRoot -Force
}
if (-not (Test-Path -LiteralPath (Join-Path $qtRoot 'lib\cmake\Qt6\Qt6Config.cmake'))) {
    Expand-Archive -LiteralPath $qtArchive -DestinationPath $qtRoot -Force
}

$obsSourceRoot = Join-Path $dependenciesRoot "obs-studio-$ObsVersion-sources"
if (-not (Test-Path -LiteralPath (Join-Path $obsSourceRoot 'CMakeLists.txt'))) {
    Invoke-NativeCommand -Command 'tar.exe' -Arguments @('-xzf', $sourceArchive, '-C', $dependenciesRoot)
}
if (-not (Test-Path -LiteralPath (Join-Path $obsSourceRoot 'CMakeLists.txt'))) {
    throw "OBS $ObsVersion source archive did not contain the expected source tree."
}

$windowsKitsInclude = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include'
$minimumSdk = $buildProfile.MinimumWindowsSdk
$sdkVersion = Get-ChildItem -LiteralPath $windowsKitsInclude -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
    ForEach-Object {
        $parsedVersion = [version]$_.Name
        if ($parsedVersion -ge $minimumSdk) { $parsedVersion }
    } |
    Sort-Object -Descending |
    Select-Object -First 1
if (-not $sdkVersion) {
    throw "OBS $ObsVersion requires Windows SDK $minimumSdk or newer; no compatible SDK was found."
}

$dependencyPrefix = "$obsDepsRoot;$qtRoot"
$obsSdkBuild = Join-Path $obsSourceRoot 'build_x64'
Invoke-NativeCommand -Command 'cmake' -Arguments @(
    '-S', $obsSourceRoot,
    '-B', $obsSdkBuild,
    '-G', 'Visual Studio 18 2026',
    '-A', "x64,version=$sdkVersion",
    '-DOBS_CMAKE_VERSION:STRING=3.0.0',
    '-DCMAKE_COMPILE_WARNING_AS_ERROR:BOOL=OFF',
    '-DENABLE_PLUGINS:BOOL=OFF',
    '-DENABLE_FRONTEND:BOOL=OFF',
    "-DOBS_VERSION_OVERRIDE:STRING=$ObsVersion",
    "-DCMAKE_PREFIX_PATH=$dependencyPrefix",
    "-DCMAKE_SYSTEM_VERSION=$sdkVersion",
    '-DCMAKE_ENABLE_SCRIPTING=OFF'
)

foreach ($configuration in @('Debug', 'Release')) {
    Invoke-NativeCommand -Command 'cmake' -Arguments @(
        '--build', $obsSdkBuild, '--target', 'obs-frontend-api',
        '--config', $configuration, '--parallel'
    )
    Invoke-NativeCommand -Command 'cmake' -Arguments @(
        '--install', $obsSdkBuild, '--component', 'Development',
        '--config', $configuration, '--prefix', $dependenciesRoot
    )
}

$pluginPrefix = "$dependenciesRoot;$dependencyPrefix"
Invoke-NativeCommand -Command 'cmake' -Arguments @(
    '-S', $projectRoot,
    '-B', $pluginBuild,
    '-G', 'Visual Studio 18 2026',
    '-A', "x64,version=$sdkVersion",
    '-DBUILD_TESTING=OFF',
    '-DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON',
    "-DCOUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT=$($buildProfile.Layout)",
    "-DCMAKE_PREFIX_PATH=$pluginPrefix"
)
Invoke-NativeCommand -Command 'cmake' -Arguments @('--build', $pluginBuild, '--config', 'Release', '--parallel')
if (-not (Get-Command 'makensis.exe' -ErrorAction SilentlyContinue) -and
    -not (Test-Path -LiteralPath (Join-Path ${env:ProgramFiles(x86)} 'NSIS\makensis.exe'))) {
    throw 'NSIS 3.12 is required to create the Windows installer.'
}
Invoke-NativeCommand -Command 'cpack' -Arguments @(
    '--config', (Join-Path $pluginBuild 'CPackConfig.cmake'),
    '-C', 'Release', '-B', $packageRoot
)

$installerSource = @(Get-ChildItem -LiteralPath $packageRoot -Filter '*.exe' -File)
$archiveSource = @(Get-ChildItem -LiteralPath $packageRoot -Filter '*.zip' -File)
if ($installerSource.Count -ne 1 -or $archiveSource.Count -ne 1) {
    throw "Expected one NSIS installer and one ZIP package in '$packageRoot'."
}

$packageStem = "CountdownStudio-$projectVersion-$($buildProfile.Family)-Windows-x64"
$installerPath = Join-Path $packageRoot "$packageStem-Setup.exe"
$archivePath = Join-Path $packageRoot "$packageStem.zip"
if ($installerSource[0].FullName -ne $installerPath) {
    Move-Item -LiteralPath $installerSource[0].FullName -Destination $installerPath -Force
}
if ($archiveSource[0].FullName -ne $archivePath) {
    Move-Item -LiteralPath $archiveSource[0].FullName -Destination $archivePath -Force
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $entries = @($zip.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
    $moduleEntry = if ($buildProfile.Layout -eq 'legacy') {
        'bin/64bit/CountdownStudio.dll'
    } else {
        'CountdownStudio.dll'
    }
    foreach ($requiredEntry in @($moduleEntry, 'data/locale/en-US.ini', 'data/locale/fr-FR.ini', 'source/LICENSE')) {
        if ($entries -notcontains $requiredEntry) {
            throw "ZIP package is missing required file '$requiredEntry'."
        }
    }
}
finally {
    $zip.Dispose()
}

$checksumPath = Join-Path $packageRoot "SHA256SUMS-$($buildProfile.Family).txt"
@($installerPath, $archivePath) |
    ForEach-Object {
        $fileHash = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant()
        "$fileHash  $(Split-Path -Leaf $_)"
    } | Set-Content -LiteralPath $checksumPath -Encoding ascii

Write-Host "Created and checked the $($buildProfile.Family) packages for OBS ${ObsVersion}:"
Get-ChildItem -LiteralPath $packageRoot -File | Select-Object Name, Length
