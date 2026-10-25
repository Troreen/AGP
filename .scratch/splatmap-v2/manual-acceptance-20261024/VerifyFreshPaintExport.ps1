param(
    [Parameter(Mandatory=$true)][string]$ExportDirectory,
    [Parameter(Mandatory=$true)][string]$EvidenceDirectory
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$scenePath = Join-Path $ExportDirectory 'Lvl_Splatmap_Testing2_Level.json'
$scene = Get-Content -LiteralPath $scenePath -Raw | ConvertFrom-Json
$landscapes = @($scene.Actors | Where-Object Archetype -eq 'Landscape')
if ($landscapes.Count -ne 1) { throw 'Expected one native Landscape.' }
$components = @($landscapes[0].Components | Where-Object TypeID -eq 12)
if ($components.Count -ne 4) { throw 'Expected four Landscape components.' }
$mesh = @($scene.Actors | ForEach-Object Components | Where-Object { $_.TypeID -eq 1 -and $_.Mesh -eq 'Lvl_Splatmap_Testing2' })
if ($mesh.Count -ne 1) { throw 'Expected exactly one fresh terrain mesh reference.' }
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$atlas = [int[]]::new(127*127*4)
for ($index=0; $index -lt $atlas.Length; $index++) { $atlas[$index] = -1 }
$records = @()
$mapping = $null
$sharedComparisons = 0
foreach ($component in $components) {
    if (@($component.Weightmaps).Count -ne 1) { throw 'Expected one packed mask per component.' }
    $meta = $component.Weightmaps[0]
    if (-not $seen.Add($meta.File)) { throw 'Mask filename collision.' }
    if ([IO.Path]::GetFileName($meta.File) -ne $meta.File) { throw 'Unexpected mask path.' }
    $layers = @($meta.Layers | Sort-Object Channel | ForEach-Object { "$($_.Name):$($_.Channel)" }) -join ','
    if ($null -eq $mapping) { $mapping = $layers }
    if ($mapping -ne $layers) { throw 'Component channel allocations differ.' }
    $imagePath = Join-Path $ExportDirectory $meta.File
    $bitmap = [Drawing.Bitmap]::new((Resolve-Path -LiteralPath $imagePath).Path)
    try {
        if ($bitmap.Width -ne 64 -or $bitmap.Height -ne 64 -or $meta.Width -ne 64 -or $meta.Height -ne 64) { throw 'Unexpected mask dimensions.' }
        $baseX = [int]$component.Transform[12]
        $baseY = [int]$component.Transform[13]
        if ($baseX -notin @(0,63) -or $baseY -notin @(0,63)) { throw 'Unexpected component layout.' }
        for ($y=0; $y -lt 64; $y++) {
            for ($x=0; $x -lt 64; $x++) {
                $color = $bitmap.GetPixel($x,$y)
                $rgba = @([int]$color.R,[int]$color.G,[int]$color.B,[int]$color.A)
                $offset = (($baseY+$y)*127+$baseX+$x)*4
                for ($c=0; $c -lt 4; $c++) {
                    if ($atlas[$offset+$c] -ge 0) {
                        $sharedComparisons++
                        if ($atlas[$offset+$c] -ne $rgba[$c]) { throw 'Shared-border RGBA mismatch.' }
                    }
                    $atlas[$offset+$c] = $rgba[$c]
                }
            }
        }
        $records += [pscustomobject]@{Component=$component.Name;File=$meta.File;X=$baseX;Y=$baseY;Layers=$meta.Layers;SHA256=(Get-FileHash -LiteralPath $imagePath).Hash}
    }
    finally { $bitmap.Dispose() }
}
if ($atlas -contains -1) { throw 'Atlas contains holes.' }
$cliffLayers = @($components[0].Weightmaps[0].Layers | Where-Object Name -eq 'Snow_Cliff')
if ($cliffLayers.Count -ne 1) { throw 'Snow_Cliff allocation missing.' }
$cliffChannel = [int]$cliffLayers[0].Channel
$patchFailures = 0
for ($y=70; $y -le 76; $y++) {
    for ($x=35; $x -le 41; $x++) {
        if ($atlas[($y*127+$x)*4+$cliffChannel] -ne 255) { $patchFailures++ }
    }
}
if ($patchFailures) { throw "Fresh paint patch missing at $patchFailures samples." }
$report = [pscustomobject]@{ActorCount=@($scene.Actors).Count;ComponentCount=$components.Count;TerrainMeshReferences=$mesh.Count;ChannelMapping=$mapping;UniqueMaskFiles=$seen.Count;SharedRGBAComparisons=$sharedComparisons;BorderMismatches=0;PatchSamples=49;PatchFailures=0;SourceSHA256=(Get-FileHash -LiteralPath $scenePath).Hash;Masks=$records}
$report | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $EvidenceDirectory '27-fresh-export-verification.json') -Encoding UTF8
$report | Format-List ActorCount,ComponentCount,TerrainMeshReferences,ChannelMapping,UniqueMaskFiles,SharedRGBAComparisons,BorderMismatches,PatchSamples,PatchFailures,SourceSHA256
