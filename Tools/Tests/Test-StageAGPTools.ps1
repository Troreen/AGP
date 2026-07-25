[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string[]]$Configuration = @('Debug', 'Release')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$stageScript = Join-Path $repoRoot 'Tools\StageAGPTools.ps1'
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("agp-tools-stage-tests-{0}" -f [guid]::NewGuid().ToString('N'))
$expectedFiles = @(
    'AGPToolsBundle.json',
    'bin/libfbxsdk.dll',
    'include/AGP/Tools/StaticMeshArtifact.h',
    'include/AGP/Tools/StaticMeshFbx.h',
    'lib/AGPTools.lib',
    'lib/libfbxsdk.lib',
    'lib/libxml2-md.lib',
    'lib/zlib-md.lib'
) | Sort-Object

try {
    foreach ($currentConfiguration in $Configuration) {
        & $stageScript -Configuration $currentConfiguration -OutputRoot $testRoot -SkipBuild
        $bundleRoot = Join-Path $testRoot "$currentConfiguration\x64"
        $manifestPath = Join-Path $bundleRoot 'AGPToolsBundle.json'
        $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json

        if ($manifest.bundle_schema_version -ne 1) { throw 'Unexpected bundle schema version.' }
        if ($manifest.tool_version -ne 'agp-static-mesh-tool/1.1.0') { throw 'Unexpected tool version.' }
        if ($manifest.artifact_schema_version -ne 1) { throw 'Unexpected artifact schema version.' }
        if ($manifest.configuration -ne $currentConfiguration -or $manifest.platform -ne 'x64') { throw 'Bundle target metadata is incorrect.' }

        $actualFiles = Get-ChildItem -LiteralPath $bundleRoot -Recurse -File |
            ForEach-Object { $_.FullName.Substring($bundleRoot.Length + 1).Replace('\', '/') } |
            Sort-Object
        if (Compare-Object -ReferenceObject $expectedFiles -DifferenceObject $actualFiles) {
            throw "Unexpected staged layout for $currentConfiguration."
        }

        foreach ($entry in $manifest.files) {
            $path = Join-Path $bundleRoot ($entry.relative_path -replace '/', '\')
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Manifest file is missing: $($entry.relative_path)" }
            if ((Get-Item -LiteralPath $path).Length -ne $entry.byte_length) { throw "Manifest length mismatch: $($entry.relative_path)" }
            if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) { throw "Manifest hash mismatch: $($entry.relative_path)" }
        }
        if (@($manifest.files).Count -ne 7) { throw 'Manifest must describe exactly seven payload files.' }
    }
}
finally {
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}

Write-Output "AGPTools staged-bundle tests passed for: $($Configuration -join ', ')"
