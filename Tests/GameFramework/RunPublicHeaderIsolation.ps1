param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [string]$MSBuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
)
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$sourceRoot = Join-Path $repository 'Source'
$frameworkRoot = Join-Path $sourceRoot 'GameFramework'
$output = Join-Path $repository "Intermediate\PublicHeaderIsolation\$Configuration"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$apiFolders = @('Runtime', 'World', 'Components', 'Scenes') | ForEach-Object { Join-Path $frameworkRoot $_ }
$headers = @(Get-ChildItem -LiteralPath $apiFolders -Recurse -Filter '*.h' | Sort-Object FullName)
if ($headers.Count -eq 0) { throw 'No public headers found.' }
$items = foreach ($header in $headers) {
    $relative = [IO.Path]::GetRelativePath($sourceRoot, $header.FullName).Replace('\','/')
    $source = Join-Path $output ($relative.Replace('/','_') + '.cpp')
    [IO.File]::WriteAllText($source, "#include <$relative>`r`n")
    '<ClCompile Include="' + [Security.SecurityElement]::Escape($source) + '" />'
}
$includes = [Security.SecurityElement]::Escape("$sourceRoot;$repository\CommonUtilities\include")
$project = @'
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="Debug|x64"><Configuration>Debug</Configuration><Platform>x64</Platform></ProjectConfiguration><ProjectConfiguration Include="Release|x64"><Configuration>Release</Configuration><Platform>x64</Platform></ProjectConfiguration></ItemGroup>
  <PropertyGroup Label="Globals"><WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion></PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Label="Configuration"><ConfigurationType>StaticLibrary</ConfigurationType><PlatformToolset>v145</PlatformToolset></PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <PropertyGroup><OutDir>$(MSBuildThisFileDirectory)bin\</OutDir><IntDir>$(MSBuildThisFileDirectory)obj\</IntDir></PropertyGroup>
  <ItemDefinitionGroup><ClCompile><LanguageStandard>stdcpp20</LanguageStandard><ExceptionHandling>Sync</ExceptionHandling><WarningLevel>Level4</WarningLevel><ShowIncludes>true</ShowIncludes><AdditionalIncludeDirectories>__INCLUDES__</AdditionalIncludeDirectories></ClCompile></ItemDefinitionGroup>
  <ItemGroup>__ITEMS__</ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
</Project>
'@
$projectPath = Join-Path $output 'PublicHeaderIsolation.vcxproj'
[IO.File]::WriteAllText($projectPath, $project.Replace('__INCLUDES__',$includes).Replace('__ITEMS__',($items -join "`r`n")))
$log = Join-Path $output 'build.log'
& $MSBuild $projectPath /t:Rebuild /m /nologo /p:Configuration=$Configuration /p:Platform=x64 /verbosity:normal *> $log
if ($LASTEXITCODE -ne 0) { Get-Content -LiteralPath $log -Tail 70; throw "Public header compilation failed: $log" }
$leaks = @(Select-String -LiteralPath $log -Pattern 'including file:.*(DirectX|[\\/]D3D[^\\/]*\.h|[\\/]Windows\.h|[\\/]RHI[\\/]|[\\/]GraphicsEngine[\\/]|[\\/]GameFramework[\\/]Rendering[\\/])')
if ($leaks.Count) { $leaks; throw 'Public header compilation imported an engine implementation header.' }
Write-Output "PASS: $($headers.Count) gameplay headers independently compile in $Configuration with Source/shared-math roots and no backend includes. Log: $log"
