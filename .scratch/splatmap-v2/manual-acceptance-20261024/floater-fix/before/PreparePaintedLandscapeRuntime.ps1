param(
    [Parameter(Mandatory = $true)][string]$SourceScene,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [Parameter(Mandatory = $true)][string]$TerrainMeshName,
    [ValidateSet('Weights','Surface')][string]$MaterialMode = 'Weights',
    [ValidateRange(0,100000)][double]$DirectionalLightIntensity,
    [double]$SurfaceRepeatMeters = 8,
    [int]$ComponentQuads = 63,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$contentRepository = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$sourcePath = (Resolve-Path -LiteralPath $SourceScene).Path
$sourceDirectory = Split-Path -Parent $sourcePath
$outputPath = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd('\')
if ($outputPath -eq $sourceDirectory -or $outputPath.StartsWith($sourceDirectory + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Use a separate output directory outside the source export.'
}
if ($ComponentQuads -lt 1) { throw 'ComponentQuads must be positive.' }
if ([double]::IsNaN($SurfaceRepeatMeters) -or [double]::IsInfinity($SurfaceRepeatMeters) -or $SurfaceRepeatMeters -le 0) {
    throw 'SurfaceRepeatMeters must be a positive finite number of metres.'
}
$surfaceRepeatCentimeters = $SurfaceRepeatMeters * 100
if ([double]::IsInfinity($surfaceRepeatCentimeters) -or $surfaceRepeatCentimeters -le 0) {
    throw 'SurfaceRepeatMeters is outside the usable centimetre range.'
}
# Match the shader's fallback world origin; this stays fixed when repeat changes.
$surfaceOriginCentimeters = -2800.0
$surfaceScale = 1 / $surfaceRepeatCentimeters
$surfacePhase = -$surfaceOriginCentimeters / $surfaceRepeatCentimeters
if ([double]::IsInfinity($surfaceScale) -or [double]::IsInfinity($surfacePhase)) {
    throw 'SurfaceRepeatMeters is too small for the surface mapping.'
}
$texconv = Join-Path $contentRepository 'texconv.exe'
if (-not (Test-Path -LiteralPath $texconv)) { throw 'texconv.exe is missing.' }
$sourceHash = (Get-FileHash -LiteralPath $sourcePath).Hash
$scene = Get-Content -LiteralPath $sourcePath -Raw | ConvertFrom-Json
$identity = @(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)

function Assert-AxisAligned($matrix, [string]$label, [switch]$UnitScale) {
    if (@($matrix).Count -ne 16) { throw "Invalid transform: $label" }
    for ($i = 0; $i -lt 16; $i++) {
        $value = [double]$matrix[$i]
        if ([double]::IsNaN($value) -or [double]::IsInfinity($value)) { throw "Nonfinite transform: $label" }
        if ($i -in @(12,13,14)) { continue }
        if ($i -in @(0,5,10) -and -not $UnitScale) {
            if ($value -le 0) { throw "Positive scale required: $label" }
        }
        elseif ([Math]::Abs($value - $identity[$i]) -gt 0.00001) {
            throw "Rotation, shear or non-unit component scale is unsupported: $label"
        }
    }
}

function Read-Weightmap($metadata) {
    $filename = [string]$metadata.File
    if ([IO.Path]::GetFileName($filename) -ne $filename) { throw 'Weightmap File must be a filename.' }
    $pngPath = Join-Path $sourceDirectory $filename
    $ddsPath = [IO.Path]::ChangeExtension($pngPath, '.dds')
    # Prefer the fresh exporter PNG over a potentially older converted DDS.
    if (Test-Path -LiteralPath $pngPath) {
        $image = [Drawing.Bitmap]::new($pngPath)
        try {
            $width = $image.Width
            $height = $image.Height
            $pixels = [int[]]::new($width * $height)
            for ($y = 0; $y -lt $height; $y++) {
                for ($x = 0; $x -lt $width; $x++) { $pixels[$y * $width + $x] = $image.GetPixel($x, $y).ToArgb() }
            }
        }
        finally { $image.Dispose() }
        $inputPath = $pngPath
    }
    elseif (Test-Path -LiteralPath $ddsPath) {
        $bytes = [IO.File]::ReadAllBytes($ddsPath)
        if ($bytes.Length -lt 148 -or [Text.Encoding]::ASCII.GetString($bytes, 0, 4) -ne 'DDS ' -or
            [Text.Encoding]::ASCII.GetString($bytes, 84, 4) -ne 'DX10' -or
            [BitConverter]::ToUInt32($bytes, 128) -ne 28 -or
            [BitConverter]::ToUInt32($bytes, 132) -ne 3 -or
            [BitConverter]::ToUInt32($bytes, 140) -ne 1) { throw 'Weightmap DDS must be 2D R8G8B8A8_UNORM.' }
        $width = [int][BitConverter]::ToUInt32($bytes, 16)
        $height = [int][BitConverter]::ToUInt32($bytes, 12)
        if ([BitConverter]::ToUInt32($bytes, 20) -ne $width * 4 -or $bytes.Length -lt 148 + $width * $height * 4) {
            throw 'Invalid weightmap DDS pitch or data size.'
        }
        $pixels = [int[]]::new($width * $height)
        for ($i = 0; $i -lt $pixels.Length; $i++) {
            $offset = 148 + $i * 4
            $pixels[$i] = [Drawing.Color]::FromArgb($bytes[$offset+3], $bytes[$offset], $bytes[$offset+1], $bytes[$offset+2]).ToArgb()
        }
        $inputPath = $ddsPath
    }
    else { throw "Missing weightmap: $filename" }
    if ($width -ne $metadata.Width -or $height -ne $metadata.Height -or
        $width -ne $ComponentQuads + 1 -or $height -ne $ComponentQuads + 1) {
        throw 'Only one-subsection components with ComponentQuads+1 samples per axis are supported.'
    }
    return [pscustomobject]@{ Width=$width; Height=$height; Pixels=$pixels; File=$inputPath; SHA256=(Get-FileHash -LiteralPath $inputPath).Hash }
}

Add-Type -AssemblyName System.Drawing
$landscapes = @($scene.Actors | Where-Object { @($_.Components | Where-Object TypeID -eq 12).Count -gt 0 })
if ($landscapes.Count -ne 1) { throw 'Expected exactly one native Landscape actor.' }
$landscape = $landscapes[0]
Assert-AxisAligned $landscape.Transform 'Landscape actor'
foreach ($component in $landscape.Components) {
    if ($component.TypeID -notin @(0,12)) { throw 'Source Landscape has additional behavior; do not discard it.' }
    Assert-AxisAligned $component.Transform $component.Name -UnitScale
    if ($component.TypeID -eq 0 -and (@($component.Transform) -join ',') -ne ($identity -join ',')) {
        throw 'Landscape root must be identity.'
    }
}
$components = @($landscape.Components | Where-Object TypeID -eq 12)
$meshComponents = @($scene.Actors | ForEach-Object Components | Where-Object { $_.TypeID -eq 1 -and $_.Mesh -eq $TerrainMeshName })
if ($meshComponents.Count -ne 1) { throw 'Expected exactly one matching reimported terrain mesh.' }
$fbxFiles = @(Get-ChildItem -LiteralPath (Join-Path $contentRepository 'Content') -Filter ($TerrainMeshName + '.fbx') -Recurse -File)
if ($fbxFiles.Count -ne 1) { throw 'Expected exactly one matching runtime FBX asset.' }
$minimumX = ($components | ForEach-Object { [double]$_.Transform[12] } | Measure-Object -Minimum).Minimum
$minimumY = ($components | ForEach-Object { [double]$_.Transform[13] } | Measure-Object -Minimum).Minimum
$maximumX = ($components | ForEach-Object { [double]$_.Transform[12] } | Measure-Object -Maximum).Maximum + $ComponentQuads
$maximumY = ($components | ForEach-Object { [double]$_.Transform[13] } | Measure-Object -Maximum).Maximum + $ComponentQuads
$width = [int]($maximumX - $minimumX + 1)
$height = [int]($maximumY - $minimumY + 1)
if ($width -gt 8192 -or $height -gt 8192) { throw 'Verification mask exceeds 8192 samples.' }
$pixels = [int[]]::new($width * $height)
$written = [bool[]]::new($pixels.Length)
$inputs = @()
$layerLayout = $null
$componentLocations = [Collections.Generic.HashSet[string]]::new()
foreach ($component in $components) {
    if (@($component.Weightmaps).Count -ne 1) { throw 'Only one packed weightmap per component is supported.' }
    $metadata = $component.Weightmaps[0]
    $channels = @($metadata.Layers | ForEach-Object { [int]$_.Channel })
    if ($channels.Count -eq 0 -or @($channels | Select-Object -Unique).Count -ne $channels.Count -or
        @($channels | Where-Object { $_ -lt 0 -or $_ -gt 3 }).Count -gt 0) { throw 'Invalid layer/channel allocation.' }
    $layout = @($metadata.Layers | Sort-Object Channel | ForEach-Object { "$($_.Channel):$($_.Name)" }) -join ','
    if (-not $layout) { throw 'Missing layer/channel metadata.' }
    if ($null -ne $layerLayout -and $layout -ne $layerLayout) { throw 'Component layer/channel layouts differ; remapping is required.' }
    $layerLayout = $layout
    $image = Read-Weightmap $metadata
    $offsetX = [double]$component.Transform[12] - $minimumX
    $offsetY = [double]$component.Transform[13] - $minimumY
    if ($offsetX % $ComponentQuads -ne 0 -or $offsetY % $ComponentQuads -ne 0) { throw 'Components are not on the expected grid.' }
    if (-not $componentLocations.Add("$offsetX,$offsetY")) { throw 'Duplicate component location.' }
    for ($y = 0; $y -lt $image.Height; $y++) {
        for ($x = 0; $x -lt $image.Width; $x++) {
            $index = ([int]$offsetY + $y) * $width + [int]$offsetX + $x
            $pixel = $image.Pixels[$y * $image.Width + $x]
            if ($written[$index] -and $pixels[$index] -ne $pixel) { throw 'Shared component border weights differ.' }
            $pixels[$index] = $pixel
            $written[$index] = $true
        }
    }
    $inputs += [pscustomobject]@{ Component=$component.Name; File=[IO.Path]::GetFileName($image.File); SHA256=$image.SHA256; Offset=@($offsetX,$offsetY) }
}
if ($written -contains $false) { throw 'The component grid contains holes.' }

# A sample at a terrain vertex must land on the centre of its atlas texel.
$scaleX = [double]$landscape.Transform[0]
$scaleY = [double]$landscape.Transform[5]
$originX = [double]$landscape.Transform[12] + $minimumX * $scaleX
$originY = [double]$landscape.Transform[13] + $minimumY * $scaleY
$maskU = @((1 / ($scaleX * $width)), 0, 0, ((0.5 - $originX / $scaleX) / $width))
$maskV = @(0, (1 / ($scaleY * $height)), 0, ((0.5 - $originY / $scaleY) / $height))
$stem = [IO.Path]::GetFileNameWithoutExtension($sourcePath) -replace '_Level$', ''
$atlasName = 'T_' + $stem + '_Weightmap_Combined'
$whiteName = 'T_' + $stem + '_Debug_BaseColor'
$materialName = 'M_PaintedLandscapeWeights'
$parameters = @(
    [pscustomobject]@{Name='MaskU';Type=1;Value=$maskU},
    [pscustomobject]@{Name='MaskV';Type=1;Value=$maskV},
    [pscustomobject]@{Name='SplatMask';Type=3;Value=@{Name=$atlasName;Path=$atlasName;AssetImportPath=($atlasName+'.dds')}},
    [pscustomobject]@{Name='AlbedoTexture';Type=3;Value=@{Name=$whiteName;Path=$whiteName;AssetImportPath=($whiteName+'.dds')}}
)
$surfaceTextures = @()
$shadingModel = 0
if ($MaterialMode -eq 'Surface') {
    $materialName = 'MM_Landscape'
    $shadingModel = 1
    # Blend order is independent of the exported mask channel order.
    $surfaceLayers = @(
        @{Role='Layer1';Paint='Snow_Dunes';Texture='T_8M_Snow_Dunes'},
        @{Role='Layer2';Paint='Snow_Rough';Texture='T_8M_Snow_Rough'},
        @{Role='Layer3';Paint='Snow_Cliff';Texture='T_8M_Snow_Cliff'}
    )
    foreach ($layer in $surfaceLayers) {
        $allocation = @($components[0].Weightmaps[0].Layers | Where-Object Name -eq $layer.Paint)
        if ($allocation.Count -ne 1) { throw "Expected one $($layer.Paint) layer for surface material." }
        if ($layer.Role -ne 'Layer1') {
            $selector = @(0,0,0,0)
            $selector[[int]$allocation[0].Channel] = 1
            $parameters += [pscustomobject]@{Name=($layer.Role+'MaskChannels');Type=1;Value=$selector}
        }
        foreach ($textureRole in @('BaseColor','ORM','Normal')) {
            $suffix = if ($textureRole -eq 'ORM') { 'OcclusionRoughnessMetallic' } else { $textureRole }
            $name = $layer.Texture+'_'+$suffix
            $surfaceTextures += $name
            $parameters += [pscustomobject]@{Name=($layer.Role+$textureRole);Type=3;Value=@{Name=$name;Path=$name;AssetImportPath=($name+'.dds')}}
        }
    }
    $noiseName = 'T_Splatmap_TransitionNoise'
    $surfaceTextures += $noiseName
    $parameters += [pscustomobject]@{Name='BlendNoise';Type=3;Value=@{Name=$noiseName;Path=$noiseName;AssetImportPath=($noiseName+'.dds')}}
    $parameters += [pscustomobject]@{Name='NoiseControls';Type=1;Value=@(0,200,0,0)}
    $parameters += [pscustomobject]@{Name='SurfaceMapping';Type=1;Value=@($surfaceScale,$surfaceScale,$surfacePhase,$surfacePhase)}
    $parameters += [pscustomobject]@{Name='DEBUG';Type=0;Value=0}
    $allDds = @(Get-ChildItem -LiteralPath (Join-Path $contentRepository 'Content') -Filter '*.dds' -Recurse -File)
    foreach ($name in $surfaceTextures) {
        $matches = @($allDds | Where-Object BaseName -eq $name)
        if ($matches.Count -ne 1) { throw "Expected one runtime DDS for $name." }
        $signature = [IO.File]::ReadAllBytes($matches[0].FullName)
        if ($signature.Length -lt 128 -or [Text.Encoding]::ASCII.GetString($signature,0,4) -ne 'DDS ') { throw "Invalid DDS: $name" }
    }
}
$meshComponents[0].Materials = @([pscustomobject]@{Name=($materialName+'_Inst');Parent=$materialName;Parameters=@()})
$scene.Actors = @($scene.Actors | Where-Object { $_ -ne $landscape })
$referencedMasters = @($scene.Actors | ForEach-Object Components | ForEach-Object Materials | ForEach-Object {
    if ($_.Parent -and $_.Parent -ne 'Self') { $_.Parent } else { $_.Name }
})
$scene.MasterMaterials = @($scene.MasterMaterials | Where-Object { $_.Name -in $referencedMasters -and $_.Name -ne $materialName })
foreach ($master in $scene.MasterMaterials) {
    foreach ($field in @('Domain','ShadingModel','BlendMode','BaseColor','Metallic','Roughness','Parameters')) {
        if ($null -eq $master.PSObject.Properties[$field]) { throw "Unresolved material hierarchy: $($master.Name)" }
    }
}
$scene.MasterMaterials += [pscustomobject]@{Name=$materialName;Domain=0;ShadingModel=$shadingModel;BlendMode=0;BaseColor=@(1,1,1,1);Metallic=0;Roughness=1;TwoSided=$false;Parameters=$parameters}

# Add a top-down camera only to the separate inspection scene.
$spanX = ($width - 1) * $scaleX
$spanY = ($height - 1) * $scaleY
$cameraHeight = [double]$landscape.Transform[14] + [Math]::Max($spanX,$spanY) * 1.2
$agpCamera = @(0,0,-1,0, 1,0,0,0, 0,-1,0,0, ($originY+$spanY/2),$cameraHeight,($originX+$spanX/2),1)
$permutation = @(2,0,1,3)
$cameraTransform = foreach ($row in $permutation) { foreach ($column in $permutation) { $agpCamera[$row*4+$column] } }
$scene.Actors = @($scene.Actors | Where-Object { @($_.Components | Where-Object TypeID -eq 11).Count -eq 0 })
$scene.Actors += [pscustomobject]@{Name='PaintedTerrainInspectionCamera';Archetype='CameraActor';Tags=@();Transform=@($cameraTransform);Components=@(
    [pscustomobject]@{Name='CameraComponent0';Parent='';TypeID=11;Tags=@();Transform=$identity;FOV=90}
)}
$lightOverride = $null
if ($PSBoundParameters.ContainsKey('DirectionalLightIntensity')) {
    $lightOverride = $DirectionalLightIntensity
    foreach ($light in @($scene.Actors | ForEach-Object Components | Where-Object TypeID -eq 5)) {
        $light.Intensity = $DirectionalLightIntensity
    }
}
$sceneName = $stem + '_Runtime_Level.json'
$rgbSceneName = $stem + '_Runtime_RGB_Level.json'
$outputFiles = @($sceneName, $rgbSceneName, ($atlasName+'.dds'), ($whiteName+'.dds'), 'preparation.json')
foreach ($file in $outputFiles) {
    if ((Test-Path -LiteralPath (Join-Path $outputPath $file)) -and -not $Force) { throw 'Output exists; use -Force to regenerate this prepared scene.' }
}
$temporaryPath = Join-Path ([IO.Path]::GetTempPath()) ('agp-painted-landscape-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporaryPath | Out-Null
try {
    $atlas = [Drawing.Bitmap]::new($width,$height,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        for ($y=0; $y -lt $height; $y++) {
            for ($x=0; $x -lt $width; $x++) { $atlas.SetPixel($x,$y,[Drawing.Color]::FromArgb($pixels[$y*$width+$x])) }
        }
        $atlas.Save((Join-Path $temporaryPath ($atlasName+'.png')),[Drawing.Imaging.ImageFormat]::Png)
    }
    finally { $atlas.Dispose() }
    $white = [Drawing.Bitmap]::new(1,1)
    try { $white.SetPixel(0,0,[Drawing.Color]::White); $white.Save((Join-Path $temporaryPath ($whiteName+'.png')),[Drawing.Imaging.ImageFormat]::Png) }
    finally { $white.Dispose() }
    foreach ($name in @($atlasName,$whiteName)) {
        & $texconv -y -f R8G8B8A8_UNORM --ignore-srgb -sepalpha -dx10 -m 0 -o $temporaryPath (Join-Path $temporaryPath ($name+'.png'))
        if ($LASTEXITCODE -ne 0) { throw 'Prepared DDS conversion failed.' }
    }
    $scene | ConvertTo-Json -Depth 50 | Set-Content -LiteralPath (Join-Path $temporaryPath $sceneName) -Encoding UTF8
    # Surface RGB mode changes only base color; all normal and ORM bindings remain.
    $rgbScene = $scene | ConvertTo-Json -Depth 50 | ConvertFrom-Json
    if ($MaterialMode -eq 'Surface') {
        $rgbMaster = $rgbScene.MasterMaterials | Where-Object Name -eq $materialName
        ($rgbMaster.Parameters | Where-Object Name -eq DEBUG).Value = 2
    }
    $rgbScene | ConvertTo-Json -Depth 50 | Set-Content -LiteralPath (Join-Path $temporaryPath $rgbSceneName) -Encoding UTF8
    [pscustomobject]@{SourceSceneSHA256=$sourceHash;Mesh=$TerrainMeshName;MeshSHA256=(Get-FileHash -LiteralPath $fbxFiles[0].FullName).Hash;Inputs=$inputs;
        Width=$width;Height=$height;Layers=$layerLayout;MaskU=$maskU;MaskV=$maskV;WorldOrigin=@($originX,$originY);WorldExtent=@($spanX,$spanY);
        MaterialMode=$MaterialMode;SurfaceTextures=$surfaceTextures;SurfaceRepeatMeters=$SurfaceRepeatMeters;
        SurfaceOriginCentimeters=@($surfaceOriginCentimeters,$surfaceOriginCentimeters);DirectionalLightIntensity=$lightOverride;
        Note='Original packed weights and explicit world mapping; no axis flip. Surface rendering requires visual verification.'} |
        ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $temporaryPath 'preparation.json') -Encoding UTF8
    if ((Get-FileHash -LiteralPath $sourcePath).Hash -ne $sourceHash) { throw 'Source changed during preparation.' }
    New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
    foreach ($file in $outputFiles) { Copy-Item -LiteralPath (Join-Path $temporaryPath $file) -Destination (Join-Path $outputPath $file) -Force:$Force }
    Write-Output ('Prepared scene: ' + (Join-Path $outputPath $sceneName))
}
finally {
    $temporaryRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    $resolvedTemporaryPath = [IO.Path]::GetFullPath($temporaryPath)
    if (-not $resolvedTemporaryPath.StartsWith($temporaryRoot,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe cleanup path.' }
    Remove-Item -LiteralPath $resolvedTemporaryPath -Recurse -Force
}
