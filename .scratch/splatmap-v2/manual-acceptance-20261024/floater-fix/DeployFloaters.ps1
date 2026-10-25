$ErrorActionPreference = 'Stop'
$engine = 'C:/Users/tarik/Documents/GitHub/AGP'
$content = 'C:/Users/tarik/Documents/GitHub/AGP-Content'
$paths = @(
    'ExportedScenes/lvl_splatmap_testing2_runtime/Lvl_Splatmap_Testing2_Runtime_Level.json',
    'ExportedScenes/lvl_splatmap_testing2_runtime/Lvl_Splatmap_Testing2_Runtime_RGB_Level.json',
    'ExportedScenes/lvl_splatmap_testing2_runtime/preparation.json'
)
$exclude = @('Textures/T_Ground_Snow_NormalMap.dds')
$exclude += Get-ChildItem -LiteralPath "$content/Content/ExportedScenes/lvl_splatmap_exporterfixture_20261025" -File | ForEach-Object { 'ExportedScenes/lvl_splatmap_exporterfixture_20261025/'+$_.Name }
& "$content/SyncContent.ps1" -EngineRoot $engine -ExcludeRelativePath $exclude -WhatIf *> "$PSScriptRoot/07-scoped-preview.log"
if (-not $?) { throw 'Scoped preview failed' }
foreach ($path in $paths) {
    $backup = Join-Path "$PSScriptRoot/before/engine" $path
    New-Item -ItemType Directory -Path (Split-Path $backup) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path "$engine/Content" $path) -Destination $backup
}
& "$content/SyncContent.ps1" -EngineRoot $engine -ExcludeRelativePath $exclude -OverwriteChangedFiles *> "$PSScriptRoot/08-deployment.log"
if (-not $?) { throw 'Deployment failed' }
$hashes = foreach ($path in $paths) {
    $source = (Get-FileHash -LiteralPath (Join-Path "$content/Content" $path)).Hash
    $destination = (Get-FileHash -LiteralPath (Join-Path "$engine/Content" $path)).Hash
    if ($source -ne $destination) { throw "Deployment mismatch: $path" }
    [pscustomobject]@{File=$path;SHA256=$destination}
}
$hashes | ConvertTo-Json | Set-Content -LiteralPath "$PSScriptRoot/08-deployed-hashes.json" -Encoding UTF8
Get-Content "$PSScriptRoot/08-deployment.log" -Tail 2
