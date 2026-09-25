param(
    [string]$FmodSdkPath
)

$ErrorActionPreference = 'Stop'
$destination = Join-Path $PSScriptRoot 'FMod\include'
$expectedHeaders = [ordered]@{
    'fmod.h' = 'F66B2A2C93BF4163306FF967DD1F7FE34AA3AE4F4715B88E6C6961B269C674E2'
    'fmod_codec.h' = 'E3C60B59CA18FF1D0E21A733421BC4512A080A929AF1A9C530E0C07436BC4C25'
    'fmod_common.h' = '609DCDA1C2D654BB96583910509C9F39853B86BC553B59013E2642325FD7634C'
    'fmod_dsp.h' = '6CFB22175187A1378C3ADF9BE01D05A8F9E1F318EA1D8F652254ABA536040130'
    'fmod_dsp_effects.h' = '4878B631AFF223E342C023F23014EFFCDE0C2724127109844D956F3E42121312'
    'fmod_errors.h' = '551D9BA12089084DC747230DB4D142A75186E2E2CE9B2D9BF1B58DB54826BFFF'
    'fmod_output.h' = '7BF2FAA13BCD0A535CEFFAC93EB5D77A6F088C2C862C6599C9282A62990B72D0'
}

function Test-Headers([string]$directory) {
    foreach ($name in $expectedHeaders.Keys) {
        $file = Join-Path $directory $name
        if (!(Test-Path -LiteralPath $file -PathType Leaf)) { return $false }
        if ((Get-FileHash -Algorithm SHA256 -LiteralPath $file).Hash -ne $expectedHeaders[$name]) { return $false }
    }
    return $true
}

if (Test-Headers $destination) {
    Write-Host 'FMOD 2.02.05 headers are ready.'
    exit 0
}

Write-Host 'This project needs headers from FMOD Studio API 2.02.05 for Windows.'
Write-Host '1. Visit https://www.fmod.com/download#fmodengine'
Write-Host '2. Choose the Windows Studio API, version 2.02.05 (patched build 123444).'
Write-Host '3. Install or extract it, then paste the SDK folder path below.'
Write-Host '   You can also paste the folder containing fmod.h.'
Write-Host '   The FMOD runtime DLLs are already in this Git checkout.'
Write-Host ''

if (!$FmodSdkPath) {
    $FmodSdkPath = Read-Host 'FMOD SDK folder (leave blank to stop)'
}
if ([string]::IsNullOrWhiteSpace($FmodSdkPath)) {
    Write-Error 'No FMOD SDK folder was supplied. Run setup again after downloading it.'
    exit 1
}

try {
    $root = (Resolve-Path -LiteralPath $FmodSdkPath -ErrorAction Stop).Path
    if (!(Test-Path -LiteralPath $root -PathType Container)) {
        throw 'The supplied path is not a folder.'
    }

    $candidates = @(
        $root,
        (Join-Path $root 'api\core\inc'),
        (Join-Path $root 'api\core\include'),
        (Join-Path $root 'core\inc'),
        (Join-Path $root 'include'),
        (Join-Path $root 'inc')
    )
    $source = $candidates | Where-Object { Test-Headers $_ } | Select-Object -First 1
    if (!$source) {
        throw 'Could not find the expected FMOD 2.02.05 headers there. Check the version and select the FMOD Studio API folder.'
    }

    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    foreach ($name in $expectedHeaders.Keys) {
        Copy-Item -LiteralPath (Join-Path $source $name) -Destination (Join-Path $destination $name) -Force
    }
    if (!(Test-Headers $destination)) {
        throw 'The copied FMOD headers did not pass the final check.'
    }
    Write-Host "FMOD headers installed in $destination."
}
catch {
    Write-Error "FMOD setup failed: $_"
    exit 1
}
