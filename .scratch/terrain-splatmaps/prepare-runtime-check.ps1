param([ValidateSet('Baseline', 'Noise', 'Weights')][string]$Variant = 'Baseline')
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
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
    $section.application.contentPath = (Join-Path $repo 'Content')
    $section.application.shaderFolder = (Join-Path $repo 'Bin/Debug/EngineShaders')
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
