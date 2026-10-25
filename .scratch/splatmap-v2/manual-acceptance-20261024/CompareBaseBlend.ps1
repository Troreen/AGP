$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$root = $PSScriptRoot
$records = @()
$total = 0
$different = 0
$maximum = 0.0
$sum = 0.0
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $root 'native-mask-readback') -Filter '*.png') {
    $image = [Drawing.Bitmap]::new($file.FullName)
    try {
        $changed = 0
        $minBase = 255
        $maxBase = 0
        for ($y=0; $y -lt $image.Height; $y++) {
            for ($x=0; $x -lt $image.Width; $x++) {
                $pixel = $image.GetPixel($x,$y)
                # Native allocation: Rough R, Dunes G, Cliff B. UE starts
                # alpha blending at zero; AGP uses a continuous Dunes base.
                $rough = $pixel.R / 255.0
                $dunes = $pixel.G / 255.0
                $cliff = $pixel.B / 255.0
                $ueRed = $dunes * (1-$rough) * (1-$cliff)
                $agpRed = (1-$rough) * (1-$cliff)
                $errorValue = [Math]::Abs($agpRed-$ueRed)
                $total++
                $sum += $errorValue
                $maximum = [Math]::Max($maximum,$errorValue)
                $minBase = [Math]::Min($minBase,$pixel.G)
                $maxBase = [Math]::Max($maxBase,$pixel.G)
                if ($errorValue -gt (1.0/255.0)) { $changed++; $different++ }
            }
        }
        $records += [pscustomobject]@{File=$file.Name; DunesMin=$minBase; DunesMax=$maxBase; DifferentOverOneByte=$changed}
    }
    finally { $image.Dispose() }
}
if ($total -ne 16384) { throw 'Native mask sample count changed.' }
$report = [pscustomobject]@{Samples=$total; DifferentOverOneByte=$different; MaxLinearRedDifference=$maximum; MeanLinearRedDifference=$sum/$total; Components=$records; EqualWithinOneByte=($different -eq 0); Scope='Base-color formula comparison using native texture readback; excludes lighting, filtering and tone mapping.'}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $root '73-native-base-blend-comparison.json') -Encoding UTF8
$report | ConvertTo-Json -Depth 5
if ($different -gt 0) { exit 2 }
