$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$export = 'C:/Users/tarik/Documents/GitHub/AGP-Content/Content/ExportedScenes/lvl_splatmap_exporterfixture_20261025'
$scene = Get-Content -LiteralPath (Join-Path $export 'Lvl_Splatmap_ExporterFixture_20261025_Level.json') -Raw | ConvertFrom-Json
$baseline = Get-Content -LiteralPath (Join-Path $PSScriptRoot '114-fixture-native-baseline.json') -Raw | ConvertFrom-Json
$source = Get-Content -LiteralPath 'C:/Users/tarik/Documents/GitHub/AGP/Content/ExportedScenes/lvl_splatmap_testing2/Lvl_Splatmap_Testing2_Level.json' -Raw | ConvertFrom-Json
if (@($scene.Actors).Count -ne 18) { throw 'Fixture actor count mismatch' }
$landscapes = @($scene.Actors | Where-Object Archetype -eq 'Landscape')
if ($landscapes.Count -ne 2) { throw 'Expected two exported proxies' }
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$maskRecords = @()
foreach ($actor in $landscapes) {
    $native = if ($actor.Name -eq 'Fixture_SharedWeightmaps') { $baseline.shared_proxy } else { $baseline.source }
    $proxyName = ($native.actor -split '\.')[-1]
    $components = @($actor.Components | Where-Object TypeID -eq 12)
    if ($components.Count -ne 4) { throw 'Proxy lost components' }
    foreach ($component in $components) {
        $nativeComponent = @($native.components | Where-Object component -eq $component.Name)
        if ($nativeComponent.Count -ne 1) { throw 'Native component lookup failed' }
        if (@($component.Weightmaps).Count -ne 1) { throw 'Expected one weightmap per component' }
        $mask = $component.Weightmaps[0]
        if (-not $seen.Add($mask.File)) { throw 'Proxy filename collision' }
        if ($mask.File -ne "${proxyName}_$($component.Name)_Weightmap_0.png") { throw 'Unexpected proxy filename' }
        foreach ($allocation in $nativeComponent[0].allocations) {
            $layer = @($mask.Layers | Where-Object Name -eq $allocation.layer)
            if ($layer.Count -ne 1 -or $layer[0].Channel -ne $allocation.channel -or $mask.TextureIndex -ne $allocation.texture_index) { throw 'Native layer allocation mismatch' }
        }
        $actual = [Drawing.Bitmap]::new((Join-Path $export $mask.File))
        $expected = [Drawing.Bitmap]::new((Join-Path $PSScriptRoot "after-sculpt-mask-readback/$($component.Name)_0.png"))
        try {
            if ($mask.Width -ne 64 -or $mask.Height -ne 64 -or $actual.Width -ne 64 -or $actual.Height -ne 64) { throw 'Mask dimensions changed' }
            for ($y=0; $y -lt 64; $y++) {
                for ($x=0; $x -lt 64; $x++) {
                    if ($actual.GetPixel($x,$y).ToArgb() -ne $expected.GetPixel($x,$y).ToArgb()) { throw 'Native fixture mask pixel mismatch' }
                }
            }
        }
        finally { $actual.Dispose(); $expected.Dispose() }
        $maskRecords += [pscustomobject]@{Proxy=$proxyName;File=$mask.File;PixelsCompared=4096;SHA256=(Get-FileHash -LiteralPath (Join-Path $export $mask.File)).Hash}
    }
}
$preserved = @()
foreach ($actor in @($source.Actors | Where-Object Archetype -ne 'Landscape')) {
    $found = @($scene.Actors | Where-Object Name -eq $actor.Name)
    if ($found.Count -ne 1 -or ($actor | ConvertTo-Json -Depth 100 -Compress) -ne ($found[0] | ConvertTo-Json -Depth 100 -Compress)) { throw "Non-Landscape actor changed: $($actor.Name)" }
    $preserved += $actor.Name
}
foreach ($material in $source.MasterMaterials) {
    $found = @($scene.MasterMaterials | Where-Object Name -eq $material.Name)
    if ($found.Count -ne 1 -or ($material | ConvertTo-Json -Depth 100 -Compress) -ne ($found[0] | ConvertTo-Json -Depth 100 -Compress)) { throw "Material changed: $($material.Name)" }
}
$camera = @($scene.Actors | Where-Object Name -eq 'Fixture_Camera')
if ($camera.Count -ne 1 -or $camera[0].Transform[12] -ne 1000 -or $camera[0].Transform[13] -ne 2000 -or $camera[0].Transform[14] -ne 9000) { throw 'Camera transform lost' }
$cameraComponent = @($camera[0].Components | Where-Object TypeID -eq 11)
if ($cameraComponent.Count -ne 1 -or $cameraComponent[0].FOV -ne 90) { throw 'Camera FOV lost' }
$behavior = @($scene.Actors | Where-Object Name -eq 'Fixture_Behavior')
$custom = @($behavior[0].Components | Where-Object TypeID -eq -1)
if ($custom.Count -ne 5) { throw 'Expected all five behavior components' }
foreach ($component in $custom) { if ($null -eq $component.Custom) { throw 'Custom behavior payload missing' } }
$empty = @($custom | Where-Object { @($_.Custom.PSObject.Properties).Count -eq 0 }).Count
$report = [pscustomobject]@{Passed=$true;Actors=18;Proxies=2;UniqueMasks=$seen.Count;NativeRGBACompared=32768;PreservedActors=$preserved;PreservedMasterMaterials=@($source.MasterMaterials).Count;CameraPreserved=$true;BehaviorComponents=$custom;EmptyCustomPayloads=$empty;EmptyPayloadGate=if($empty) {'observed'} else {'not exercised by available actor'};Masks=$maskRecords}
$report | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath (Join-Path $PSScriptRoot '116-exporter-fixture-verification.json') -Encoding UTF8
[pscustomobject]@{Passed=$true;Proxies=2;Masks=$seen.Count;PixelsCompared=32768;PreservedOrdinaryActors=$preserved.Count;BehaviorComponents=$custom.Count;EmptyPayloads=$empty}
