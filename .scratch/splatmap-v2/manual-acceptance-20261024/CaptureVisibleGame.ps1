param([int]$GameProcessId=2324,[string]$OutputStem='46-visible-game-window')
$ErrorActionPreference='Stop'
if($OutputStem -notmatch '^[A-Za-z0-9_-]+$') { throw 'Invalid evidence filename' }
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class ReadOnlyWindowCapture
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr window, IntPtr context, uint flags);
}
'@
$game=Get-Process -Id $GameProcessId
if($game.Path -ne 'C:\Users\tarik\Documents\GitHub\AGP\Bin\Debug\Game.exe' -or $game.MainWindowHandle -eq 0) { throw 'Expected visible AGP test window' }
$rect=New-Object ReadOnlyWindowCapture+Rect
if(-not [ReadOnlyWindowCapture]::GetWindowRect($game.MainWindowHandle,[ref]$rect)) { throw 'Cannot read window bounds' }
$width=$rect.Right-$rect.Left;$height=$rect.Bottom-$rect.Top
$bitmap=[Drawing.Bitmap]::new($width,$height)
$graphics=[Drawing.Graphics]::FromImage($bitmap)
$context=$graphics.GetHdc()
try { $captured=[ReadOnlyWindowCapture]::PrintWindow($game.MainWindowHandle,$context,2) }
finally { $graphics.ReleaseHdc($context) }
$destination=Join-Path $PSScriptRoot ($OutputStem+'.png')
try {
    $bitmap.Save($destination,[Drawing.Imaging.ImageFormat]::Png)
    $colors=[Collections.Generic.HashSet[int]]::new()
    for($y=0;$y -lt $height;$y+=16) { for($x=0;$x -lt $width;$x+=16) { [void]$colors.Add($bitmap.GetPixel($x,$y).ToArgb()) } }
    $report=[pscustomobject]@{ProcessId=$game.Id;Title=$game.MainWindowTitle;Width=$width;Height=$height;PrintWindowSucceeded=$captured;SampledUniqueColors=$colors.Count;Screenshot=$destination;CapturedAt=(Get-Date -Format o)}
    $report | ConvertTo-Json | Set-Content (Join-Path $PSScriptRoot ($OutputStem+'.json')) -Encoding UTF8
    $report | Format-List
} finally { $graphics.Dispose();$bitmap.Dispose() }
