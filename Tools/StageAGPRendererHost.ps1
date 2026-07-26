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
    $OutputRoot = Join-Path $repoRoot 'Artifacts\AGPRendererHost'
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
    $found = & $vswhere -latest -products '*' -find 'MSBuild\**\Bin\amd64\MSBuild.exe' | Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($found)) {
        throw 'Could not locate 64-bit MSBuild. Set AGP_MSBUILD_PATH explicitly.'
    }
    return $found
}

function Invoke-AGPProjectBuild([string]$Project) {
    $solutionDir = $repoRoot.TrimEnd('\') + '\\'
    $command = 'set "PATH=" & "{0}" "{1}" /m /p:Configuration={2} /p:Platform=x64 /p:SolutionDir="{3}" /v:minimal' -f $script:msbuild, $Project, $Configuration, $solutionDir
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) {
        throw "$Project $Configuration x64 build failed with exit code $LASTEXITCODE."
    }
}

if (-not $SkipBuild) {
    $script:msbuild = Find-MSBuild
    Invoke-AGPProjectBuild (Join-Path $repoRoot 'Source\Utilities\Logger\Logger.vcxproj')
    Invoke-AGPProjectBuild (Join-Path $repoRoot 'Source\GameFramework\GameFramework.vcxproj')
    Invoke-AGPProjectBuild (Join-Path $repoRoot 'Source\Graphics\GraphicsEngine\GraphicsEngine.vcxproj')
}

$commonUtilitiesLibrary = if ($Configuration -eq 'Debug') { 'CommonUtilities-d.lib' } else { 'CommonUtilities.lib' }
$files = @(
    @{ Source = (Join-Path $repoRoot 'Source\Graphics\GraphicsEngine\RendererHost.h'); Relative = 'include/AGP/RendererHost.h'; Role = 'public_header' },
    @{ Source = (Join-Path $repoRoot "Lib\$Configuration\GraphicsEngine.lib"); Relative = 'lib/GraphicsEngine.lib'; Role = 'static_library' },
    @{ Source = (Join-Path $repoRoot "Lib\$Configuration\GameFramework.lib"); Relative = 'lib/GameFramework.lib'; Role = 'internal_link_dependency' },
    @{ Source = (Join-Path $repoRoot "Lib\$Configuration\Logger.lib"); Relative = 'lib/Logger.lib'; Role = 'static_library' },
    @{ Source = (Join-Path $repoRoot "CommonUtilities\lib\$commonUtilitiesLibrary"); Relative = 'lib/CommonUtilities.lib'; Role = 'static_library' },
    @{ Source = (Join-Path $repoRoot 'Assets\Textures\T_Shipyard.dds'); Relative = 'fixtures/T_Shipyard.dds'; Role = 'test_fixture' }
)
$shaderRoot = Join-Path $repoRoot 'Source\Graphics\GraphicsEngine\Shaders'
foreach ($shader in Get-ChildItem -LiteralPath $shaderRoot -Recurse -File | Sort-Object FullName) {
    $relativeShader = $shader.FullName.Substring($shaderRoot.Length).TrimStart('\').Replace('\', '/')
    $files += @{ Source = $shader.FullName; Relative = "shaders/$relativeShader"; Role = 'shader_source' }
}

foreach ($file in $files) {
    if (-not (Test-Path -LiteralPath $file.Source -PathType Leaf)) {
        throw "Required renderer-host bundle input is missing: $($file.Source)"
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

$headerText = Get-Content -LiteralPath (Join-Path $repoRoot 'Source\Graphics\GraphicsEngine\RendererHost.h') -Raw
$versionMatch = [regex]::Match($headerText, 'RendererHostVersion\s*=\s*"([^"]+)"')
if (-not $versionMatch.Success) {
    throw 'Could not read renderer-host version metadata from RendererHost.h.'
}

$manifest = [ordered]@{
    bundle_schema_version = 1
    host_version = $versionMatch.Groups[1].Value
    configuration = $Configuration
    platform = 'x64'
    system_link_libraries = @('d3d11.lib', 'dxguid.lib', 'dxgi.lib', 'd3dcompiler.lib')
    files = @($manifestFiles)
}
$manifestPath = Join-Path $bundleRoot 'AGPRendererHostBundle.json'
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding utf8

foreach ($entry in $manifest.files) {
    $path = Join-Path $bundleRoot ($entry.relative_path -replace '/', '\')
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) {
        throw "Staged bundle hash verification failed: $($entry.relative_path)"
    }
}

Write-Output "Staged AGP renderer host $Configuration x64 bundle: $bundleRoot"
