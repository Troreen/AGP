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

$solutionDir = $repoRoot.TrimEnd('\') + '\\'
$bundleRoot = Join-Path $repoRoot "Artifacts\AGPRendererHost\$Configuration\x64"
$faultRoot = Join-Path $repoRoot "Artifacts\RendererHostFaultTests\$Configuration\x64"
$faultLibraryRoot = Join-Path $faultRoot 'lib'
$faultObjectRoot = Join-Path $faultRoot 'obj'
$graphicsProject = Join-Path $repoRoot 'Source\Graphics\GraphicsEngine\GraphicsEngine.vcxproj'
$graphicsCommand = 'set "PATH=" & "{0}" "{1}" /m /p:Configuration={2} /p:Platform=x64 /p:SolutionDir="{3}" /p:RendererHostFaultInjection=true /p:TargetName=GraphicsEngineFaultInjection /p:OutDir="{4}\\" /p:IntDir="{5}\\" /v:minimal' -f $msbuild, $graphicsProject, $Configuration, $solutionDir, $faultLibraryRoot, $faultObjectRoot
& $env:ComSpec /d /c $graphicsCommand
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$testProject = Join-Path $repoRoot 'Source\Tools\RendererHostFaultTests\RendererHostFaultTests.vcxproj'
$testCommand = 'set "PATH=" & "{0}" "{1}" /m /p:Configuration={2} /p:Platform=x64 /p:SolutionDir="{3}" /p:RendererHostBundleRoot="{4}\\" /p:RendererHostFaultLibraryRoot="{5}\\" /v:minimal' -f $msbuild, $testProject, $Configuration, $solutionDir, $bundleRoot, $faultLibraryRoot
& $env:ComSpec /d /c $testCommand
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$testExecutable = Join-Path $repoRoot "Bin\$Configuration\RendererHostFaultTests.exe"
$shaderRoot = Join-Path $bundleRoot 'shaders'
& $testExecutable $shaderRoot 'partial-init-retry'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $testExecutable $shaderRoot 'resize-recovery'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $testExecutable $shaderRoot 'scene-resource-preparation'
exit $LASTEXITCODE
