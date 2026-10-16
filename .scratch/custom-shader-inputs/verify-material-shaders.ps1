param(
    [string]$Fxc = 'C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/fxc.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$outputRoot = Join-Path $repoRoot 'Intermediate/CustomShaderVerification'
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

# Compile through the public material hook interface in both shader sets.
$fixture = @'
#include "MaterialParameters.hlsli"
void Material_Vertex(inout MaterialVertexParameters p) {}
void Material_Pixel(inout MaterialPixelParameters p)
{
    p.PixelColor = p.VertexColor;
    p.PixelColor.rg *= p.UV1;
    p.SurfaceValues = float4(0.25f, 0.65f, 0.85f, 0.0f);
}
'@

$shaderSets = @{
    engine = 'Source/Engine/GraphicsEngine/Shaders/Material'
    content = 'Content/Shaders/Material'
}
foreach ($set in $shaderSets.GetEnumerator())
{
    $destination = Join-Path $outputRoot $set.Key
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    Copy-Item -Path (Join-Path (Join-Path $repoRoot $set.Value) '*') -Destination $destination -Force

    $variants = if ($set.Key -eq 'engine') { @('default', 'custom', 'MM_ShaderTest', 'MM_Standard_WithSnow') } else { @('default', 'custom', '_DefaultMaterial') }
    foreach ($variant in $variants)
    {
        if ($variant -eq 'custom')
        {
            Set-Content -LiteralPath (Join-Path $destination 'Material.hlsli') -Value $fixture -Encoding ASCII
        }
        elseif ($variant -ne 'default')
        {
            $customPath = Join-Path (Split-Path (Join-Path $repoRoot $set.Value)) ($variant + '.hlsli')
            Copy-Item -LiteralPath $customPath -Destination (Join-Path $destination 'Material.hlsli') -Force
        }
        foreach ($shader in @('Lit_PS', 'GBuffer_PS', 'Unlit_PS', 'Surface_VS'))
        {
            # This legacy include already conflicts with Lighting's b4 LightBuffer.
            if ($variant -eq '_DefaultMaterial' -and $shader -in @('Lit_PS', 'Unlit_PS')) { continue }
            $target = if ($shader.EndsWith('_VS')) { 'vs_5_0' } else { 'ps_5_0' }
            $outputFile = Join-Path $destination ($variant + '-' + $shader + '.cso')
            & $Fxc /nologo /T $target /E main /Fo $outputFile (Join-Path $destination ($shader + '.hlsl'))
            if ($LASTEXITCODE -ne 0) { throw ($set.Key + '/' + $variant + '/' + $shader + ' failed') }
            Write-Output ($set.Key + '/' + $variant + '/' + $shader + ' passed')
        }
    }
}

$engineMaterialPath = Join-Path $repoRoot $shaderSets.engine
foreach ($shader in @('Deferred_AmbientLight_PS', 'Deferred_DirLight_PS', 'Deferred_PointLight_PS', 'Deferred_SpotLight_PS'))
{
    & $Fxc /nologo /T ps_5_0 /E main /Fo (Join-Path $outputRoot ($shader + '.cso')) (Join-Path $engineMaterialPath ($shader + '.hlsl'))
    if ($LASTEXITCODE -ne 0) { throw ($shader + ' failed') }
    Write-Output ($shader + ' passed')
}
