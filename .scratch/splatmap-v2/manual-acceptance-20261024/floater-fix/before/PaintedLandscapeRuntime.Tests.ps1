$ErrorActionPreference = 'Stop'
$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$prepare = Join-Path $repositoryRoot 'Tools/Terrain/PreparePaintedLandscapeRuntime.ps1'
$temporaryRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
$fixture = Join-Path $temporaryRoot ('agp-painted-test-' + [guid]::NewGuid().ToString('N'))
Add-Type -AssemblyName System.Drawing
function Check($condition, [string]$message) { if (-not $condition) { throw $message } }
function Invoke-Preparation([string]$source, [string]$output, [string]$failure = '', [string]$mode = 'Weights', [string[]]$extraArguments = @()) {
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $log = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $prepare -SourceScene $source -OutputDirectory $output -TerrainMeshName Lvl_Splatmap_Testing2 -ComponentQuads 2 -MaterialMode $mode @extraArguments 2>&1
        $exitCode = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $previousPreference }
    if ($failure) {
        Check ($exitCode -ne 0 -and ($log -join "`n") -match $failure) "Expected failure '$failure': $log"
    }
    else { Check ($exitCode -eq 0) "Preparation failed: $log" }
}
try {
    $inputDirectory = Join-Path $fixture 'source'
    New-Item -ItemType Directory -Path $inputDirectory -Force | Out-Null
    $identity = @(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)
    $landscapeTransform = @(100,0,0,0,0,100,0,0,0,0,100,0,-100,-100,0,1)
    $components = @()
    for ($i=0; $i -lt 4; $i++) {
        $offsetX = ($i % 2) * 2
        $offsetY = [Math]::Floor($i / 2) * 2
        $transform = $identity.Clone()
        $transform[12] = $offsetX
        $transform[13] = $offsetY
        $filename = "Component_${i}_Weightmap_0.png"
        $image = [Drawing.Bitmap]::new(3,3,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            for ($y=0; $y -lt 3; $y++) {
                for ($x=0; $x -lt 3; $x++) {
                    $globalX = $offsetX + $x
                    $globalY = $offsetY + $y
                    $image.SetPixel($x,$y,[Drawing.Color]::FromArgb(($globalX*20+$globalY),($globalX*30),($globalY*40),($globalX*10+$globalY*5)))
                }
            }
            $image.Save((Join-Path $inputDirectory $filename),[Drawing.Imaging.ImageFormat]::Png)
        }
        finally { $image.Dispose() }
        $components += @{Name="Component_$i";TypeID=12;Parent='';Tags=@();Transform=$transform;Weightmaps=@(
            @{File=$filename;Width=3;Height=3;TextureIndex=0;Layers=@(@{Name='Snow_Rough';Channel=0},@{Name='Snow_Dunes';Channel=1},@{Name='Snow_Cliff';Channel=2})}
        )}
    }
    $source = Join-Path $inputDirectory 'Fixture_Level.json'
    $scene = @{MasterMaterials=@(@{Name='MM_Landscape';Domain=0;ShadingModel=1;BlendMode=0;BaseColor=@(1,1,1,1);Metallic=0;Roughness=1;Parameters=@()});Actors=@(
        @{Name='Landscape';Archetype='Landscape';Tags=@();Transform=$landscapeTransform;Components=$components},
        @{Name='Terrain';Archetype='StaticMeshActor';Tags=@();Transform=$identity;Components=@(
            @{Name='Mesh';Parent='';TypeID=1;Tags=@();Transform=$identity;Mesh='Lvl_Splatmap_Testing2';Materials=@()}
        )},
        @{Name='Light';Archetype='DirectionalLight';Tags=@();Transform=$identity;Components=@(
            @{Name='LightComponent';Parent='';TypeID=5;Tags=@();Transform=$identity;Intensity=10}
        )}
    )}
    # Preparation must keep unrelated materials and non-spatial behavior intact.
    $ordinaryMaster = @{Name='MM_Ordinary';Domain=0;ShadingModel=1;BlendMode=0;BaseColor=@(0.2,0.3,0.4,1);Metallic=0.25;Roughness=0.75;TwoSided=$true;Parameters=@()}
    $ordinaryActor = @{Name='PreservedActor';Archetype='StaticMeshActor';Tags=@('Keep');Transform=$identity;Components=@(
        @{Name='OrdinaryMesh';Parent='';TypeID=1;Tags=@();Transform=$identity;Mesh='OrdinaryMesh';Materials=@(@{Name='OrdinaryInstance';Parent='MM_Ordinary';Parameters=@()})},
        @{Name='EmptyBehavior';Parent='';TypeID=-1;CustomType='EmptyBehavior';Tags=@();Transform=$identity;Data=@{}},
        @{Name='Behavior';Parent='';TypeID=-1;CustomType='Behavior';Tags=@();Transform=$identity;Data=@{Enabled=$true;Count=3}}
    )}
    $skyActor = @{Name='PreservedSky';Archetype='SkyLight';Tags=@();Transform=$identity;Components=@(
        @{Name='Sky';Parent='';TypeID=10;Tags=@();Transform=$identity;Intensity=0.75;Cubemap=@{Name='SkyCube';CubemapAngle=37}}
    )}
    $scene.MasterMaterials += $ordinaryMaster
    $scene.Actors += @($ordinaryActor,$skyActor)
    $preservedActorJson = $ordinaryActor | ConvertTo-Json -Depth 20 -Compress
    $preservedSkyJson = $skyActor | ConvertTo-Json -Depth 20 -Compress
    $preservedMasterJson = $ordinaryMaster | ConvertTo-Json -Depth 20 -Compress
    $scene | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $source -Encoding UTF8
    $sourceHash = (Get-FileHash -LiteralPath $source).Hash
    $output = Join-Path $fixture 'prepared'
    Invoke-Preparation $source $output
    $dds = [IO.File]::ReadAllBytes((Join-Path $output 'T_Fixture_Weightmap_Combined.dds'))
    Check ([BitConverter]::ToUInt32($dds,16) -eq 5 -and [BitConverter]::ToUInt32($dds,12) -eq 5) 'Atlas dimensions differ.'
    for ($y=0; $y -lt 5; $y++) {
        for ($x=0; $x -lt 5; $x++) {
            $offset=148+($y*5+$x)*4
            Check ($dds[$offset] -eq $x*30 -and $dds[$offset+1] -eq $y*40 -and
                $dds[$offset+2] -eq $x*10+$y*5 -and $dds[$offset+3] -eq $x*20+$y) 'RGBA samples were changed or misplaced.'
        }
    }
    $prepared = Get-Content -LiteralPath (Join-Path $output 'Fixture_Runtime_Level.json') -Raw | ConvertFrom-Json
    Check (@($prepared.Actors | ForEach-Object Components | Where-Object TypeID -eq 12).Count -eq 0) 'Native terrain leaked into prepared scene.'
    Check (@($prepared.Actors | ForEach-Object Components | Where-Object TypeID -eq 11).Count -eq 1) 'Inspection camera missing.'
    $weightsMaster = $prepared.MasterMaterials | Where-Object Name -eq M_PaintedLandscapeWeights
    Check ($weightsMaster.ShadingModel -eq 0) 'Debug material is not unlit.'
    $u = ($weightsMaster.Parameters | Where-Object Name -eq MaskU).Value
    Check ([Math]::Abs((-100*$u[0]+$u[3])-0.1) -lt 0.000001) 'First vertex does not sample the first texel centre.'
    Check ([Math]::Abs((300*$u[0]+$u[3])-0.9) -lt 0.000001) 'Last vertex does not sample the last texel centre.'
    Check ((Get-FileHash -LiteralPath $source).Hash -eq $sourceHash) 'Source JSON changed.'
    $surfaceOutput = Join-Path $fixture 'surface'
    Invoke-Preparation $source $surfaceOutput '' 'Surface' @('-DirectionalLightIntensity','2')
    $surface = Get-Content -LiteralPath (Join-Path $surfaceOutput 'Fixture_Runtime_Level.json') -Raw | ConvertFrom-Json
    $surfaceMaster = $surface.MasterMaterials | Where-Object Name -eq MM_Landscape
    Check (@($surfaceMaster).Count -eq 1) 'Duplicate Landscape masters can hide the prepared bindings.'
    Check ($surfaceMaster.ShadingModel -eq 1) 'Surface material is not lit.'
    foreach ($inspectionScene in @($prepared,$surface)) {
        Check ((($inspectionScene.Actors | Where-Object Name -eq PreservedActor | ConvertTo-Json -Depth 20 -Compress)) -eq $preservedActorJson) 'Preparation changed ordinary mesh or custom behavior payloads.'
        Check ((($inspectionScene.Actors | Where-Object Name -eq PreservedSky | ConvertTo-Json -Depth 20 -Compress)) -eq $preservedSkyJson) 'Preparation changed skylight or cubemap rotation.'
        Check ((($inspectionScene.MasterMaterials | Where-Object Name -eq MM_Ordinary | ConvertTo-Json -Depth 20 -Compress)) -eq $preservedMasterJson) 'Preparation changed ordinary material defaults or TwoSided.'
    }
    Check (@($surfaceMaster.Parameters | Where-Object Type -eq 3).Count -eq 12) 'Surface texture roles are missing.'
    Check ((($surfaceMaster.Parameters | Where-Object Name -eq Layer2MaskChannels).Value -join ',') -eq '1,0,0,0') 'Layer2 does not select the exported Rough red channel.'
    Check ((($surfaceMaster.Parameters | Where-Object Name -eq Layer3MaskChannels).Value -join ',') -eq '0,0,1,0') 'Layer3 does not select the exported Cliff blue channel.'
    foreach ($binding in @(@('Layer1BaseColor','T_8M_Snow_Dunes_BaseColor'),@('Layer2BaseColor','T_8M_Snow_Rough_BaseColor'),@('Layer3BaseColor','T_8M_Snow_Cliff_BaseColor'))) {
        Check (($surfaceMaster.Parameters | Where-Object Name -eq $binding[0]).Value.Name -eq $binding[1]) "Incorrect surface asset for $($binding[0])."
    }
    Check (($surfaceMaster.Parameters | Where-Object Name -eq DEBUG).Value -eq 0) 'Surface mode left debug colors enabled.'
    $defaultMapping = ($surfaceMaster.Parameters | Where-Object Name -eq SurfaceMapping).Value
    Check (($defaultMapping -join ',') -eq '0.00125,0.00125,3.5,3.5') 'Default eight-metre surface mapping changed.'
    $repeatOutput = Join-Path $fixture 'surface-16m'
    Invoke-Preparation $source $repeatOutput '' 'Surface' @('-SurfaceRepeatMeters','16')
    $repeatScene = Get-Content -LiteralPath (Join-Path $repeatOutput 'Fixture_Runtime_Level.json') -Raw | ConvertFrom-Json
    $repeatMapping = (($repeatScene.MasterMaterials | Where-Object Name -eq MM_Landscape).Parameters | Where-Object Name -eq SurfaceMapping).Value
    Check (($repeatMapping -join ',') -eq '0.000625,0.000625,1.75,1.75') 'Sixteen-metre repeat did not update scale and phase together.'
    Check ((Get-Content -LiteralPath (Join-Path $repeatOutput 'preparation.json') -Raw | ConvertFrom-Json).SurfaceRepeatMeters -eq 16) 'Repeat size is absent from provenance.'
    Invoke-Preparation $source (Join-Path $fixture 'invalid-repeat-zero') 'positive finite' 'Surface' @('-SurfaceRepeatMeters','0')
    Invoke-Preparation $source (Join-Path $fixture 'invalid-repeat-nan') 'positive finite' 'Surface' @('-SurfaceRepeatMeters','NaN')
    $rgbScene = Get-Content -LiteralPath (Join-Path $surfaceOutput 'Fixture_Runtime_RGB_Level.json') -Raw | ConvertFrom-Json
    $rgbMaster = @($rgbScene.MasterMaterials | Where-Object Name -eq MM_Landscape)
    Check ($rgbMaster.Count -eq 1 -and $rgbMaster[0].ShadingModel -eq 1) 'RGB base-color view lost the lit Landscape material.'
    Check (($rgbMaster[0].Parameters | Where-Object Name -eq DEBUG).Value -eq 2) 'RGB base-color view did not select raw weights.'
    Check (@($rgbMaster[0].Parameters | Where-Object Type -eq 3).Count -eq 12) 'RGB base-color view lost normal/ORM/surface texture bindings.'
    Check ((($rgbScene.Actors | ForEach-Object Components | Where-Object Mesh -eq Lvl_Splatmap_Testing2).Materials[0].Parent) -eq 'MM_Landscape') 'RGB base-color view uses the wrong material.'
    Check (($surface.Actors | ForEach-Object Components | Where-Object TypeID -eq 5).Intensity -eq 2) 'Explicit inspection light intensity was lost.'
    Check (($prepared.Actors | ForEach-Object Components | Where-Object TypeID -eq 5).Intensity -eq 10) 'Default preparation changed source lighting.'
    Invoke-Preparation $source $output 'Output exists'
    Invoke-Preparation $source $inputDirectory 'separate output'
    # ExportAssets removes runtime PNGs after conversion; preparation must also work from DDS.
    foreach ($png in Get-ChildItem -LiteralPath $inputDirectory -Filter '*.png') {
        & (Join-Path $repositoryRoot 'texconv.exe') -y -f R8G8B8A8_UNORM --ignore-srgb -sepalpha -dx10 -m 0 -o $inputDirectory $png.FullName | Out-Null
        Check ($LASTEXITCODE -eq 0) 'DDS-only fixture conversion failed.'
        Move-Item -LiteralPath $png.FullName -Destination ($png.FullName+'.saved')
    }
    $ddsOutput = Join-Path $fixture 'dds-only'
    Invoke-Preparation $source $ddsOutput
    Check ((Get-FileHash -LiteralPath (Join-Path $ddsOutput 'T_Fixture_Weightmap_Combined.dds')).Hash -eq
        (Get-FileHash -LiteralPath (Join-Path $output 'T_Fixture_Weightmap_Combined.dds')).Hash) 'DDS input changed combined weights.'
    foreach ($saved in Get-ChildItem -LiteralPath $inputDirectory -Filter '*.saved') {
        Move-Item -LiteralPath $saved.FullName -Destination ($saved.FullName -replace '\.saved$','')
    }
    $badBorder = Join-Path $inputDirectory 'Component_1_Weightmap_0.png'
    $changed = [Drawing.Bitmap]::new($badBorder)
    $changedPath = Join-Path $inputDirectory 'changed.png'
    try { $changed.SetPixel(0,0,[Drawing.Color]::Magenta); $changed.Save($changedPath,[Drawing.Imaging.ImageFormat]::Png) }
    finally { $changed.Dispose() }
    Copy-Item -LiteralPath $badBorder -Destination ($badBorder+'.saved')
    Copy-Item -LiteralPath $changedPath -Destination $badBorder -Force
    Invoke-Preparation $source (Join-Path $fixture 'invalid-border') 'border weights differ'
    Copy-Item -LiteralPath ($badBorder+'.saved') -Destination $badBorder -Force
    $scene.Actors[0].Components[1].Weightmaps[0].Layers[0].Name='Other'
    $scene | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $source -Encoding UTF8
    Invoke-Preparation $source (Join-Path $fixture 'invalid-layout') 'layouts differ'
    $scene.Actors[0].Components[1].Weightmaps[0].Layers[0].Name='Snow_Rough'
    $scene.Actors[0].Components = @($scene.Actors[0].Components[0..2])
    $scene | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $source -Encoding UTF8
    Invoke-Preparation $source (Join-Path $fixture 'invalid-hole') 'grid contains holes'
    Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'invalid-hole'))) 'Invalid source created output.'
    Write-Output 'PASS: exact RGBA atlas placement, texel centres, scene/material/camera preparation, ordinary material/behavior/skylight preservation, source preservation, overwrite/layout/hole guards.'
}
finally {
    $resolvedFixture = [IO.Path]::GetFullPath($fixture)
    if (-not $resolvedFixture.StartsWith($temporaryRoot,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe fixture cleanup.' }
    if (Test-Path -LiteralPath $resolvedFixture) { Remove-Item -LiteralPath $resolvedFixture -Recurse -Force }
}
