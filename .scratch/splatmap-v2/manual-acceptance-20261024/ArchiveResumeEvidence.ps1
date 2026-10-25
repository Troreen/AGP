$ErrorActionPreference = 'Stop'
$engineRoot = 'C:/Users/tarik/Documents/GitHub/AGP'
$destination = 'C:/Users/tarik/Documents/GitHub/AGP-Content/.scratch/splatmap-v2/validation/manual-acceptance-20261024/resume-20261025'
if (Test-Path -LiteralPath $destination) { throw 'Snapshot already exists; refuse overwrite.' }
$files = @(Get-ChildItem -LiteralPath $PSScriptRoot -File)
foreach ($folder in @('native-mask-readback','acceptance-helper')) {
    $files += Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot $folder) -File -Recurse
}
$entries = @($files | ForEach-Object {
    [pscustomobject]@{Source=$_.FullName; Relative='evidence/'+$_.FullName.Substring($PSScriptRoot.Length+1)}
})
$entries += Get-ChildItem -LiteralPath (Join-Path $engineRoot 'Intermediate/SplatmapV2Verification') -File |
    Where-Object Extension -in '.cpp','.h','.vcxproj','.ps1' | ForEach-Object {
        [pscustomobject]@{Source=$_.FullName; Relative='probes/'+$_.Name}
    }
foreach ($name in @('SplatmapV2AcceptanceRun-2026-10-24.md','SplatmapV2NextChat.md','SplatmapV2ManualAcceptance.md')) {
    $entries += [pscustomobject]@{Source=(Join-Path $engineRoot "Docs/$name"); Relative="Docs/$name"}
}
$manifest = foreach ($entry in $entries) {
    $target = Join-Path $destination $entry.Relative
    New-Item -ItemType Directory -Path (Split-Path $target) -Force | Out-Null
    $sourceHash = (Get-FileHash -LiteralPath $entry.Source).Hash
    Copy-Item -LiteralPath $entry.Source -Destination $target
    $copyHash = (Get-FileHash -LiteralPath $target).Hash
    if ($sourceHash -ne $copyHash) { throw "Copy mismatch: $($entry.Relative)" }
    [pscustomobject]@{File=$entry.Relative; SHA256=$copyHash; Bytes=(Get-Item -LiteralPath $target).Length}
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $destination 'manifest.json') -Encoding UTF8
$summary = [pscustomobject]@{Destination=$destination; Files=@($manifest).Count; Bytes=($manifest | Measure-Object Bytes -Sum).Sum; AllHashesMatch=$true}
$summary | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot '84-resume-archive-result.json') -Encoding UTF8
$summary | ConvertTo-Json
