$ErrorActionPreference = 'Stop'
$engineRoot = 'C:/Users/tarik/Documents/GitHub/AGP'
$sceneNames = @(
    'lvl_splatmap_testing2_runtime/Lvl_Splatmap_Testing2_Runtime_Level.json',
    'lvl_splatmap_testing2_runtime/Lvl_Splatmap_Testing2_Runtime_RGB_Level.json',
    'lvl_splatmap_testing2_inspection/Lvl_Splatmap_Testing2_Contributions_Level.json'
)
$reports = foreach ($name in $sceneNames) {
    $scene = Get-Content -LiteralPath (Join-Path $engineRoot "Content/ExportedScenes/$name") -Raw | ConvertFrom-Json
    $terrain = @($scene.Actors | Where-Object Name -eq 'Lvl_Splatmap_Testing2')[0]
    $terrainMaterial = @($terrain.Components | Where-Object TypeID -eq 1)[0].Materials[0]
    $floaters = foreach ($actor in @($scene.Actors | Where-Object Name -match '^SM_Cube\d*$')) {
        $material = @($actor.Components | Where-Object TypeID -eq 1)[0].Materials[0]
        [pscustomobject]@{Actor=$actor.Name;Material=$material.Name;Parent=$material.Parent;SharesGroundMaterial=($material.Parent -eq $terrainMaterial.Parent)}
    }
    if (@($floaters).Count -ne 12) { throw 'Expected twelve known test floaters' }
    [pscustomobject]@{Scene=$name;TerrainMaterial=$terrainMaterial.Name;TerrainParent=$terrainMaterial.Parent;Floaters=$floaters;Passed=(@($floaters | Where-Object { -not $_.SharesGroundMaterial }).Count -eq 0)}
}
$reports | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $PSScriptRoot '122-ground-floater-material-audit.json') -Encoding UTF8
$reports | ForEach-Object { [pscustomobject]@{Scene=$_.Scene;Floaters=@($_.Floaters).Count;ShareGroundMaterial=@($_.Floaters | Where-Object SharesGroundMaterial).Count;Passed=$_.Passed} }
if (@($reports | Where-Object { -not $_.Passed }).Count -gt 0) { exit 2 }
