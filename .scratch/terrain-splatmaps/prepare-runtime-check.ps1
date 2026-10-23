param([ValidateSet('Baseline', 'Noise', 'Weights', 'NoiseWeights')][string]$Variant = 'Baseline')
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if ($Variant -eq 'NoiseWeights')
{
    # Reuse the RGB baseline's geometry and camera; enable noise on every material.
    $sceneRoot = Join-Path $repo 'Content/ExportedScenes/lvl_splatmap_runtime'
    $scene = Get-Content -LiteralPath (Join-Path $sceneRoot 'Lvl_Splatmap_Runtime_Weights_Level.json') -Raw | ConvertFrom-Json
    $enableNoise = {
        param($node)
        if ($node -is [System.Management.Automation.PSCustomObject])
        {
            if ($node.Name -eq 'NoiseControls' -and $node.Type -eq 1)
            {
                $node.Value[0] = 0.75
            }
            foreach ($property in $node.PSObject.Properties)
            {
                & $enableNoise $property.Value
            }
        }
        elseif ($node -is [array])
        {
            foreach ($item in $node) { & $enableNoise $item }
        }
    }
    & $enableNoise $scene
    $scene | ConvertTo-Json -Depth 50 | Set-Content -LiteralPath (Join-Path $sceneRoot 'Lvl_Splatmap_Runtime_NoiseWeights_Level.json') -Encoding UTF8
}
$runtimeName = if ($Variant -eq 'Baseline') { 'TerrainVerificationRuntime' } else { 'TerrainVerification' + $Variant }
$runtime = Join-Path $repo ('Intermediate/' + $runtimeName)
New-Item -ItemType Directory -Force -Path (Join-Path $runtime 'Settings') | Out-Null
foreach ($file in @(Get-ChildItem -LiteralPath (Join-Path $repo 'Bin/Debug') -File))
{
    if ($file.Extension -in @('.exe', '.dll'))
    {
        Copy-Item -LiteralPath $file.FullName -Destination $runtime -Force
    }
}
foreach ($file in @(Get-ChildItem -LiteralPath (Join-Path $repo 'Bin/Settings') -File))
{
    Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $runtime 'Settings') -Force
}
$audio = Join-Path $repo 'Bin/Debug/Audio'
if (Test-Path -LiteralPath $audio)
{
    Copy-Item -LiteralPath $audio -Destination $runtime -Recurse -Force
}
$settingsPath = Join-Path $runtime 'Settings/ApplicationSettings.json'
$settings = Get-Content -LiteralPath $settingsPath -Raw | ConvertFrom-Json
foreach ($section in @($settings.defaults, $settings.current))
{
    $section.application.contentPath = '../../Content'
    $section.application.shaderFolder = '../../Bin/Debug/EngineShaders'
    $sceneName = if ($Variant -eq 'Baseline') { 'Lvl_Splatmap_Runtime_Level.json' } else { 'Lvl_Splatmap_Runtime_' + $Variant + '_Level.json' }
    $section.application.initialScene = 'lvl_splatmap_runtime/' + $sceneName
    $section.application.title = 'AGP Terrain Verification ' + $Variant
    $section.application.windowClassName = 'AGP Terrain Verification ' + $Variant
    $section.application.windowMode = 'windowed'
    $section.application.windowedResolution.width = 1280
    $section.application.windowedResolution.height = 720
    $section.sound.masterVolume = 0
}
$settings | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $settingsPath -Encoding UTF8
Write-Output ('Isolated runtime prepared: ' + (Join-Path $runtime 'Game.exe'))
