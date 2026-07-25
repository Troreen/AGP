[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [string]$OutputRoot,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $repoRoot 'Artifacts\AGPTools'
}
$resolvedOutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
$bundleRoot = [System.IO.Path]::GetFullPath((Join-Path $resolvedOutputRoot "$Configuration\x64"))

if (-not $bundleRoot.StartsWith($resolvedOutputRoot + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Bundle target escaped the requested output root: $bundleRoot"
}

function Find-MSBuild {
    if (-not [string]::IsNullOrWhiteSpace($env:AGP_MSBUILD_PATH)) {
        if (-not (Test-Path -LiteralPath $env:AGP_MSBUILD_PATH -PathType Leaf)) {
            throw "AGP_MSBUILD_PATH does not name a file: $env:AGP_MSBUILD_PATH"
        }
        return $env:AGP_MSBUILD_PATH
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere -PathType Leaf) {
        $found = & $vswhere -latest -products '*' -find 'MSBuild\**\Bin\amd64\MSBuild.exe' | Select-Object -First 1
        if (-not [string]::IsNullOrWhiteSpace($found)) {
            return $found
        }
    }

    $visualStudioRoot = Join-Path $env:ProgramFiles 'Microsoft Visual Studio'
    $found = Get-ChildItem -LiteralPath $visualStudioRoot -Recurse -Filter MSBuild.exe -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\Bin\\amd64\\MSBuild\.exe$' } |
        Sort-Object FullName -Descending |
        Select-Object -First 1 -ExpandProperty FullName
    if ([string]::IsNullOrWhiteSpace($found)) {
        throw 'Could not locate 64-bit MSBuild. Set AGP_MSBUILD_PATH explicitly.'
    }
    return $found
}

if (-not $SkipBuild) {
    $msbuild = Find-MSBuild
    $project = Join-Path $repoRoot 'Source\Tools\AGPTools\AGPTools.vcxproj'
    # Double the trailing separator so MSBuild's command-line parser does not
    # consume the closing quote as part of a path ending in a backslash.
    $solutionDir = $repoRoot.TrimEnd('\') + '\\'
    $command = 'set "PATH=" & "{0}" "{1}" /m /p:Configuration={2} /p:Platform=x64 /p:SolutionDir="{3}" /v:minimal' -f $msbuild, $project, $Configuration, $solutionDir
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) {
        throw "AGPTools $Configuration x64 build failed with exit code $LASTEXITCODE."
    }
}

$publicHeader = Join-Path $repoRoot 'Source\Tools\AGPTools\StaticMeshArtifact.h'
$fbxHeader = Join-Path $repoRoot 'Source\Tools\AGPTools\StaticMeshFbx.h'
$configurationKey = $Configuration.ToLowerInvariant()
$files = @(
    @{ Source = $publicHeader; Relative = 'include/AGP/Tools/StaticMeshArtifact.h'; Role = 'public_header' },
    @{ Source = $fbxHeader; Relative = 'include/AGP/Tools/StaticMeshFbx.h'; Role = 'public_header' },
    @{ Source = (Join-Path $repoRoot "Lib\$Configuration\AGPTools.lib"); Relative = 'lib/AGPTools.lib'; Role = 'static_library' },
    @{ Source = (Join-Path $repoRoot "ThirdParty\TGAFBXImporter\FBXSDK\lib\$configurationKey\libfbxsdk.lib"); Relative = 'lib/libfbxsdk.lib'; Role = 'link_library' },
    @{ Source = (Join-Path $repoRoot "ThirdParty\TGAFBXImporter\FBXSDK\lib\$configurationKey\libxml2-md.lib"); Relative = 'lib/libxml2-md.lib'; Role = 'link_library' },
    @{ Source = (Join-Path $repoRoot "ThirdParty\TGAFBXImporter\FBXSDK\lib\$configurationKey\zlib-md.lib"); Relative = 'lib/zlib-md.lib'; Role = 'link_library' },
    @{ Source = (Join-Path $repoRoot "Bin\$Configuration\libfbxsdk.dll"); Relative = 'bin/libfbxsdk.dll'; Role = 'runtime' }
)

foreach ($file in $files) {
    if (-not (Test-Path -LiteralPath $file.Source -PathType Leaf)) {
        throw "Required AGPTools bundle input is missing: $($file.Source)"
    }
}

if (Test-Path -LiteralPath $bundleRoot) {
    Remove-Item -LiteralPath $bundleRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $bundleRoot -Force | Out-Null

$manifestFiles = foreach ($file in $files | Sort-Object { $_.Relative }) {
    $destination = Join-Path $bundleRoot ($file.Relative -replace '/', '\')
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $file.Source -Destination $destination
    $copied = Get-Item -LiteralPath $destination
    [ordered]@{
        relative_path = $file.Relative
        role = $file.Role
        byte_length = [uint64]$copied.Length
        sha256 = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

$headerText = Get-Content -LiteralPath $publicHeader -Raw
$toolMatch = [regex]::Match($headerText, 'StaticMeshToolVersion\s*=\s*"([^"]+)"')
$schemaMatch = [regex]::Match($headerText, 'StaticMeshArtifactSchemaVersion\s*=\s*([0-9]+)')
if (-not $toolMatch.Success -or -not $schemaMatch.Success) {
    throw 'Could not read AGPTools version metadata from StaticMeshArtifact.h.'
}

$manifest = [ordered]@{
    bundle_schema_version = 1
    tool_version = $toolMatch.Groups[1].Value
    artifact_schema_version = [uint32]$schemaMatch.Groups[1].Value
    configuration = $Configuration
    platform = 'x64'
    files = @($manifestFiles)
}
$manifestPath = Join-Path $bundleRoot 'AGPToolsBundle.json'
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding utf8

foreach ($entry in $manifest.files) {
    $path = Join-Path $bundleRoot ($entry.relative_path -replace '/', '\')
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) {
        throw "Staged bundle hash verification failed: $($entry.relative_path)"
    }
}

Write-Output "Staged AGPTools $Configuration x64 bundle: $bundleRoot"
