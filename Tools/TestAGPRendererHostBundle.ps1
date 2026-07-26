[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
& (Join-Path $PSScriptRoot 'StageAGPRendererHost.ps1') -Configuration $Configuration

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$msbuild = & $vswhere -latest -products '*' -find 'MSBuild\**\Bin\amd64\MSBuild.exe' | Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($msbuild)) {
    throw 'Could not locate 64-bit MSBuild.'
}
$project = Join-Path $repoRoot 'Source\Tools\RendererHostBundleTests\RendererHostBundleTests.vcxproj'
$solutionDir = $repoRoot.TrimEnd('\') + '\\'
$bundleRoot = Join-Path $repoRoot "Artifacts\AGPRendererHost\$Configuration\x64"
$command = 'set "PATH=" & "{0}" "{1}" /m /p:Configuration={2} /p:Platform=x64 /p:SolutionDir="{3}" /p:RendererHostBundleRoot="{4}\\" /v:minimal' -f $msbuild, $project, $Configuration, $solutionDir, $bundleRoot
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$testExecutable = Join-Path $repoRoot "Bin\$Configuration\RendererHostBundleTests.exe"
& $testExecutable (Join-Path $bundleRoot 'shaders') 'partial-init-retry'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $testExecutable (Join-Path $bundleRoot 'shaders') 'normal'
exit $LASTEXITCODE
