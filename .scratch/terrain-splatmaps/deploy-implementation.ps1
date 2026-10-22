[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ContentRoot,
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [switch]$WhatIf
)
$ErrorActionPreference = 'Stop'
$stagingRoot = Join-Path $PSScriptRoot 'content-staging'
$ContentRoot = [IO.Path]::GetFullPath($ContentRoot)
$EngineRoot = [IO.Path]::GetFullPath($EngineRoot)
$files = @(Get-ChildItem -LiteralPath $stagingRoot -Recurse -File)
$copies = @()
foreach ($file in $files)
{
    $relative = $file.FullName.Substring($stagingRoot.Length + 1)
    $destinations = @(Join-Path $ContentRoot $relative)
    if ($relative.StartsWith('Content\'))
    {
        $destinations += Join-Path $EngineRoot $relative
    }
    foreach ($destination in $destinations)
    {
        $resolved = [IO.Path]::GetFullPath($destination)
        if (-not ($resolved.StartsWith($ContentRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or
                  $resolved.StartsWith($EngineRoot + '\Content\', [StringComparison]::OrdinalIgnoreCase)))
        {
            throw "Destination escapes its intended root: $resolved"
        }
        if (Test-Path -LiteralPath $resolved)
        {
            if ((Get-FileHash -LiteralPath $resolved).Hash -ne (Get-FileHash -LiteralPath $file.FullName).Hash)
            {
                throw "Existing destination differs; inspect it before any replacement: $resolved"
            }
        }
        else
        {
            $copies += @{Source = $file.FullName; Destination = $resolved; Relative = $relative}
        }
    }
}
foreach ($copy in $copies)
{
    Write-Output ('Copy ' + $copy.Relative + ' -> ' + $copy.Destination)
    if (-not $WhatIf)
    {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $copy.Destination) | Out-Null
        Copy-Item -LiteralPath $copy.Source -Destination $copy.Destination
        if ((Get-FileHash -LiteralPath $copy.Source).Hash -ne (Get-FileHash -LiteralPath $copy.Destination).Hash)
        {
            throw 'Deployment checksum mismatch'
        }
    }
}
Write-Output ("Scoped deployment: $($copies.Count) new files; existing equal files preserved")
