$ErrorActionPreference = 'Stop'
$staging = Join-Path $PSScriptRoot 'content-staging'
$content = 'C:\Users\tarik.bergstrom\Perforce\AGP-Content'
$engine = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
# Only these files created by this run changed after the initial scoped copy.
$replaceable = @(
    'Tools\Terrain\ExportTerrainRuntime.py',
    'Tools\Terrain\BuildTerrainScene.py',
    'Tools\Terrain\BuildTerrainScene.md',
    'Content\ExportedScenes\lvl_splatmap_runtime\Lvl_Splatmap_Runtime_Level.provenance.json'
)
$copies = @()
foreach ($file in @(Get-ChildItem -LiteralPath $staging -Recurse -File))
{
    $relative = $file.FullName.Substring($staging.Length + 1)
    $destinations = @(Join-Path $content $relative)
    if ($relative.StartsWith('Content\')) { $destinations += Join-Path $engine $relative }
    foreach ($destination in $destinations)
    {
        if (Test-Path -LiteralPath $destination)
        {
            if ((Get-FileHash -LiteralPath $destination).Hash -eq (Get-FileHash -LiteralPath $file.FullName).Hash) { continue }
            if ($relative -notin $replaceable) { throw "Unexpected differing existing file: $destination" }
        }
        $copies += @{Source = $file.FullName; Destination = $destination}
    }
}
foreach ($copy in $copies)
{
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $copy.Destination) | Out-Null
    Copy-Item -LiteralPath $copy.Source -Destination $copy.Destination -Force
    if ((Get-FileHash -LiteralPath $copy.Source).Hash -ne (Get-FileHash -LiteralPath $copy.Destination).Hash) { throw 'Copy mismatch' }
    Write-Output $copy.Destination
}
