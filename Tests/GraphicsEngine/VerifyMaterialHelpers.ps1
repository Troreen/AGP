param(
    [string]$Fxc = 'C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/fxc.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$outputRoot = Join-Path $repoRoot 'Intermediate/MaterialHelperVerification'
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

# Work on copies so the fixture never replaces the game's default material.
Copy-Item -Path (Join-Path $repoRoot 'Source/Engine/GraphicsEngine/Shaders/Material/*') -Destination $outputRoot -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'MaterialHelpersFixture.hlsli') -Destination (Join-Path $outputRoot 'Material.hlsli') -Force

# Check opaque, forward, unlit, and vertex compilation. Materials are included by all four.
foreach ($shader in @('GBuffer_PS', 'Lit_PS', 'Unlit_PS', 'Surface_VS'))
{
    $target = if ($shader.EndsWith('_VS')) { 'vs_5_0' } else { 'ps_5_0' }
    & $Fxc /nologo /WX /T $target /E main /Fo (Join-Path $outputRoot ($shader + '.cso')) (Join-Path $outputRoot ($shader + '.hlsl'))
    if ($LASTEXITCODE -ne 0) { throw ($shader + ' material helper compilation failed') }
    Write-Output ($shader + ': material helper fixture passed')
}
