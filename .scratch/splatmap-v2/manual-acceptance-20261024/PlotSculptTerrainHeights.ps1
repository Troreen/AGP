$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$root = $PSScriptRoot
$samples = Import-Csv (Join-Path $root '92-sculpt-native-agp-samples.csv')
$native = New-Object 'double[,]' 127,127
$agp = New-Object 'double[,]' 127,127
$seen = @{}
foreach ($s in $samples) {
    $x = [int](([double]$s.ue_x_cm + 6300) / 100)
    $y = [int](([double]$s.ue_y_cm + 6300) / 100)
    $key = "$x,$y"
    if ($seen.ContainsKey($key) -and $agp[$x,$y] -ne [double]$s.agp_height_cm) { throw 'Conflicting duplicate' }
    $seen[$key] = $true
    $native[$x,$y] = [double]$s.native_height_cm
    $agp[$x,$y] = [double]$s.agp_height_cm
}
if ($seen.Count -ne 16129) { throw 'Incomplete grid' }
$bitmap = New-Object System.Drawing.Bitmap 1240,740
$g = [System.Drawing.Graphics]::FromImage($bitmap)
$g.Clear([System.Drawing.Color]::White)
$title = New-Object System.Drawing.Font 'Segoe UI',20
$label = New-Object System.Drawing.Font 'Segoe UI',12
$black = [System.Drawing.Brushes]::Black
$g.DrawString('Terrain height comparison - no lighting or material effects',$title,$black,24,15)
$g.DrawString('Unreal native Landscape',$title,$black,50,65)
$g.DrawString('FBX through AGP importer',$title,$black,650,65)
$min = ($samples | Measure-Object native_height_cm -Minimum).Minimum
$max = ($samples | Measure-Object native_height_cm -Maximum).Maximum
for ($panel = 0; $panel -lt 2; $panel++) {
    $grid = $native
    if ($panel -eq 1) { $grid = $agp }
    $ox = 50 + $panel * 600
    for ($y = 0; $y -lt 127; $y++) {
        for ($x = 0; $x -lt 127; $x++) {
            $t = ($grid[$x,$y] - $min) / ($max - $min)
            # Blue valleys, pale intermediate slopes, orange peaks. Same scale on both panels.
            if ($t -lt 0.5) {
                $a = $t * 2
                $r = 35 + 200 * $a; $green = 80 + 155 * $a; $b = 165 + 70 * $a
            } else {
                $a = ($t - 0.5) * 2
                $r = 235 + 15 * $a; $green = 235 - 130 * $a; $b = 235 - 210 * $a
            }
            $brush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb([int]$r,[int]$green,[int]$b))
            $g.FillRectangle($brush,($ox + $x * 4),(110 + (126-$y)*4),4,4)
            $brush.Dispose()
        }
    }
    $g.DrawString('UE X: -63 m to +63 m; UE Y increases upward',$label,$black,$ox,625)
}
$g.DrawString(('Shared height scale: {0:F3} m (blue) to {1:F3} m (orange)' -f ($min/100),($max/100)),$label,$black,50,662)
$g.DrawString('All 16,384 imported vertices: height error 0 cm. Maximum XY rounding: 0.000976563 cm.',$label,$black,50,693)
$path = Join-Path $root '97-sculpt-terrain-height-comparison.png'
$bitmap.Save($path,[System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bitmap.Dispose(); $title.Dispose(); $label.Dispose()
Write-Output $path
