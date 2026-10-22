<#
.SYNOPSIS
Prepares linear terrain weights and transition noise without changing source PNGs.
.DESCRIPTION
Requires DirectXTex texconv with --ignore-srgb support. Produces RGBA8 UNORM DDS
textures with complete mip chains, a top-level PNG round trip, and verification.json.
All four source channels must survive both conversions exactly before publishing
outputs. Existing output files require -Force. No source files are removed.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string] $MaskPng,
    [Parameter(Mandatory = $true)][string] $NoisePng,
    [Parameter(Mandatory = $true)][string] $TexconvPath,
    [Parameter(Mandatory = $true)][string] $OutputDirectory,
    [switch] $Force
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

foreach ($inputPath in @($MaskPng, $NoisePng, $TexconvPath))
{
    if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf))
    {
        throw "Input file does not exist: $inputPath"
    }
}

$MaskPng = (Resolve-Path -LiteralPath $MaskPng).Path
$NoisePng = (Resolve-Path -LiteralPath $NoisePng).Path
$TexconvPath = (Resolve-Path -LiteralPath $TexconvPath).Path
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$textureInputs = @(
    @{ Source = $MaskPng; Name = 'T_Splatmap_LevelWeights'; Wrap = $false },
    @{ Source = $NoisePng; Name = 'T_Splatmap_TransitionNoise'; Wrap = $true }
)

# Check the whole publication set first, so a refused overwrite changes nothing.
$outputNames = @('verification.json')
foreach ($textureInput in $textureInputs)
{
    $outputNames += "$($textureInput.Name).dds"
    $outputNames += "$($textureInput.Name).roundtrip.png"
}
foreach ($outputName in $outputNames)
{
    $destination = Join-Path $OutputDirectory $outputName
    if (($destination -eq $MaskPng) -or ($destination -eq $NoisePng))
    {
        throw "An output would replace an original source: $destination"
    }
    if ((Test-Path -LiteralPath $destination) -and -not $Force)
    {
        throw "Output already exists; use -Force to replace prepared outputs: $destination"
    }
}

Add-Type -AssemblyName System.Drawing
if (-not ('TerrainTexturePixels' -as [type]))
{
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System.Drawing;
public static class TerrainTexturePixels
{
    public static byte[] ReadRgba(string path)
    {
        using (var bitmap = new Bitmap(path))
        {
            var pixels = new byte[checked(bitmap.Width * bitmap.Height * 4)];
            int index = 0;
            for (int y = 0; y < bitmap.Height; ++y)
            {
                for (int x = 0; x < bitmap.Width; ++x)
                {
                    Color pixel = bitmap.GetPixel(x, y);
                    pixels[index++] = pixel.R;
                    pixels[index++] = pixel.G;
                    pixels[index++] = pixel.B;
                    pixels[index++] = pixel.A;
                }
            }
            return pixels;
        }
    }
    public static int[] Compare(byte[] source, byte[] candidate, int offset)
    {
        if (candidate.Length - offset < source.Length)
            throw new System.ArgumentException("Candidate pixel buffer is too short.");
        var differences = new int[4];
        for (int i = 0; i < source.Length; ++i)
        {
            if (source[i] != candidate[i + offset])
                ++differences[i % 4];
        }
        return differences;
    }
}
'@
}

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$workDirectory = Join-Path $OutputDirectory ('.prepare-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $workDirectory | Out-Null
$results = @()
try
{
    foreach ($textureInput in $textureInputs)
    {
        $sourceHash = (Get-FileHash -LiteralPath $textureInput.Source -Algorithm SHA256).Hash
        $sourceCopy = Join-Path $workDirectory ($textureInput.Name + '.png')
        Copy-Item -LiteralPath $textureInput.Source -Destination $sourceCopy
        $arguments = @('-nologo', '--ignore-srgb', '-f', 'R8G8B8A8_UNORM', '-dx10', '-m', '0', '-if', 'BOX', '-nowic', '-sepalpha')
        if ($textureInput.Wrap)
        {
            $arguments += '-wrap'
        }
        & $TexconvPath @arguments '-o' $workDirectory $sourceCopy
        if ($LASTEXITCODE -ne 0)
        {
            throw "DDS conversion failed: $($textureInput.Source)"
        }

        $ddsPath = Join-Path $workDirectory ($textureInput.Name + '.dds')
        $ddsBytes = [IO.File]::ReadAllBytes($ddsPath)
        if (($ddsBytes.Length -lt 148) -or ([Text.Encoding]::ASCII.GetString($ddsBytes, 0, 4) -ne 'DDS ') -or
            ([Text.Encoding]::ASCII.GetString($ddsBytes, 84, 4) -ne 'DX10'))
        {
            throw "Expected a DDS with the DX10 header: $ddsPath"
        }
        $width = [BitConverter]::ToUInt32($ddsBytes, 16)
        $height = [BitConverter]::ToUInt32($ddsBytes, 12)
        $mipCount = [BitConverter]::ToUInt32($ddsBytes, 28)
        $dxgiFormat = [BitConverter]::ToUInt32($ddsBytes, 128)
        $expectedMipCount = 1
        $expectedSize = 148L
        $mipWidth = $width
        $mipHeight = $height
        while ($true)
        {
            $expectedSize += [long]$mipWidth * $mipHeight * 4
            if (($mipWidth -eq 1) -and ($mipHeight -eq 1))
            {
                break
            }
            $mipWidth = [Math]::Max(1, [Math]::Floor($mipWidth / 2))
            $mipHeight = [Math]::Max(1, [Math]::Floor($mipHeight / 2))
            ++$expectedMipCount
        }
        if (($dxgiFormat -ne 28) -or ($mipCount -ne $expectedMipCount) -or ($ddsBytes.Length -ne $expectedSize))
        {
            throw "DDS is not linear RGBA8 with a complete, correctly sized mip chain: $ddsPath"
        }

        # Compare raw DDS bytes as well as a decoder round trip. This also catches
        # a flip, channel swap, gamma conversion, or loss of RGB beneath zero alpha.
        $roundtripDirectory = Join-Path $workDirectory $textureInput.Name
        New-Item -ItemType Directory -Path $roundtripDirectory | Out-Null
        & $TexconvPath '-nologo' '-ft' 'png' '-m' '1' '-o' $roundtripDirectory $ddsPath
        if ($LASTEXITCODE -ne 0)
        {
            throw "DDS to PNG verification conversion failed: $ddsPath"
        }
        $roundtripPath = Join-Path $roundtripDirectory ($textureInput.Name + '.png')
        $sourcePixels = [TerrainTexturePixels]::ReadRgba($sourceCopy)
        $roundtripPixels = [TerrainTexturePixels]::ReadRgba($roundtripPath)
        $sourceBitmap = [Drawing.Bitmap]::new($sourceCopy)
        try
        {
            if (($sourceBitmap.Width -ne $width) -or ($sourceBitmap.Height -ne $height) -or
                ($roundtripPixels.Length -ne $sourcePixels.Length))
            {
                throw 'Texture dimensions changed during conversion.'
            }
        }
        finally
        {
            $sourceBitmap.Dispose()
        }
        $ddsDifferences = [TerrainTexturePixels]::Compare($sourcePixels, $ddsBytes, 148)
        $roundtripDifferences = [TerrainTexturePixels]::Compare($sourcePixels, $roundtripPixels, 0)
        if ((($ddsDifferences | Measure-Object -Sum).Sum -ne 0) -or
            (($roundtripDifferences | Measure-Object -Sum).Sum -ne 0))
        {
            throw "Top-level RGBA bytes changed: $($textureInput.Name)"
        }
        if ((Get-FileHash -LiteralPath $textureInput.Source -Algorithm SHA256).Hash -ne $sourceHash)
        {
            throw "Source changed during preparation: $($textureInput.Source)"
        }
        $results += [ordered]@{
            source = $textureInput.Source; sourceSha256 = $sourceHash
            output = $textureInput.Name + '.dds'; width = $width; height = $height
            dxgiFormat = $dxgiFormat; mipCount = $mipCount; fileBytes = $ddsBytes.Length
            ddsChannelMismatchCountsRGBA = $ddsDifferences
            roundtripChannelMismatchCountsRGBA = $roundtripDifferences
            sourceUnchanged = $true
        }
    }

    # Publish only after both textures pass. Temporary diagnostics are retained
    # on failure so a failed conversion can be inspected without touching inputs.
    foreach ($textureInput in $textureInputs)
    {
        Copy-Item -LiteralPath (Join-Path $workDirectory ($textureInput.Name + '.dds')) -Destination $OutputDirectory -Force:$Force
        $roundtripPath = Join-Path (Join-Path $workDirectory $textureInput.Name) ($textureInput.Name + '.png')
        Copy-Item -LiteralPath $roundtripPath -Destination (Join-Path $OutputDirectory ($textureInput.Name + '.roundtrip.png')) -Force:$Force
    }
    [ordered]@{ preparedUtc = [DateTime]::UtcNow.ToString('o'); textures = $results } |
        ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'verification.json') -Encoding UTF8
    Write-Output "Prepared and verified textures in $OutputDirectory"
}
catch
{
    Write-Warning "Preparation diagnostics retained in $workDirectory"
    throw
}
