[CmdletBinding()]
param(
    [string]$Label,
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [ValidateRange(1, 1000000)]
    [int]$WarmupFrames = 300,
    [ValidateRange(1, 10000000)]
    [int]$SampleFrames = 1200,
    [ValidateRange(1, 20)]
    [int]$Repetitions = 10,
    [ValidateSet("default", "busy")]
    [string]$Scenario = "default",
    [string]$ComparisonId,
    [ValidateRange(1, 1000000)]
    [int]$ExecutionIndex = 1,
    [ValidateRange(1, 1000000)]
    [int]$RunIndex = 1,
    [string]$RequestedRef,
    [string]$Notes = "",
    [string]$RepoRoot,
    [string]$ResultsRoot,
    [string]$CommitOverride,
    [string]$BranchOverride,
    [string]$Harness = "in-tree",
    [Nullable[bool]]$DirtyOverride,
    [switch]$BuildOnly,
    [switch]$NoBuild,
    [switch]$NoReport
)

$ErrorActionPreference = "Stop"
if (-not $RepoRoot) {
    $RepoRoot = Split-Path -Parent $PSScriptRoot
}
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
if (-not $ResultsRoot) {
    $ResultsRoot = Join-Path $RepoRoot "Benchmarks\results"
}
$ResultsRoot = [System.IO.Path]::GetFullPath($ResultsRoot)

function Invoke-GitText {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    $text = & git -C $RepoRoot @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git $($Arguments -join ' ') failed."
    }
    return ($text | Out-String).Trim()
}

function Find-MSBuild {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" |
            Select-Object -First 1
        if ($found -and (Test-Path -LiteralPath $found)) {
            return $found
        }
    }

    $knownCandidates = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    )
    foreach ($candidate in $knownCandidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "Could not find MSBuild. Install Visual Studio with the Desktop development with C++ workload."
}

function Remove-CreatedJunction {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    $item = Get-Item -LiteralPath $Path -Force
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -eq 0) {
        throw "Refusing to remove temporary path because it is not a junction: $($item.FullName)"
    }

    [System.IO.Directory]::Delete($item.FullName, $false)
}

if (-not $CommitOverride) {
    $CommitOverride = Invoke-GitText -Arguments @("rev-parse", "HEAD")
}
if (-not $BranchOverride) {
    $BranchOverride = Invoke-GitText -Arguments @("branch", "--show-current")
    if (-not $BranchOverride) {
        $BranchOverride = "detached"
    }
}
if (-not $Label) {
    $Label = $BranchOverride
}
if (-not $RequestedRef) {
    $RequestedRef = $Label
}
if (-not $ComparisonId) {
    $ComparisonId = [Guid]::NewGuid().ToString("N")
}

if ($null -eq $DirtyOverride) {
    $Dirty = [bool](Invoke-GitText -Arguments @("status", "--porcelain", "--untracked-files=no"))
}
else {
    $Dirty = $DirtyOverride.Value
}

if (-not $NoBuild) {
    $msbuild = Find-MSBuild
    $solution = Join-Path $RepoRoot "AGP.sln"
    Write-Host "Building ModelViewer ($Configuration x64)..."

    # A child cmd with one canonical, deduplicated PATH avoids the duplicate
    # Path/PATH issue seen in this checkout while retaining tools such as xcopy.
    $pathEntries = $env:Path -split ';' |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object { $_.Trim().TrimEnd('\') } |
        Select-Object -Unique
    $sanitizedPath = $pathEntries -join ';'
    $buildCommand = 'set PATH=&& set "PATH=' + $sanitizedPath + '" && "' + $msbuild + '" "' + $solution +
        '" /t:ModelViewer /p:Configuration=' + $Configuration + ' /p:Platform=x64 /m /nologo'
    & $env:ComSpec /d /s /c $buildCommand
    if ($LASTEXITCODE -ne 0) {
        throw "ModelViewer build failed with exit code $LASTEXITCODE."
    }
}

if ($BuildOnly) {
    Write-Host "Build complete: $RepoRoot"
    return
}

$executable = Join-Path $RepoRoot "Bin\$Configuration\ModelViewer.exe"
if (-not (Test-Path -LiteralPath $executable)) {
    throw "ModelViewer executable not found at $executable."
}
$builtShaderDirectory = Join-Path $RepoRoot "Bin\$Configuration\Shaders"
if (-not (Test-Path -LiteralPath $builtShaderDirectory -PathType Container)) {
    throw "Built shader directory not found at $builtShaderDirectory."
}
$runtimeShaderDirectory = Join-Path $RepoRoot "Assets\Shaders"
$createdShaderJunction = $false
if (-not (Test-Path -LiteralPath $runtimeShaderDirectory)) {
    New-Item -ItemType Junction -Path $runtimeShaderDirectory -Target $builtShaderDirectory | Out-Null
    $createdShaderJunction = $true
    Write-Host "Created temporary runtime Shaders junction."
}

New-Item -ItemType Directory -Force -Path $ResultsRoot | Out-Null

$benchmarkVariables = @(
    "AGP_BENCHMARK",
    "AGP_BENCHMARK_OUTPUT",
    "AGP_BENCHMARK_LABEL",
    "AGP_BENCHMARK_COMMIT",
    "AGP_BENCHMARK_BRANCH",
    "AGP_BENCHMARK_CONFIGURATION",
    "AGP_BENCHMARK_RUN",
    "AGP_BENCHMARK_SCENARIO",
    "AGP_BENCHMARK_COMPARISON_ID",
    "AGP_BENCHMARK_EXECUTION_INDEX",
    "AGP_BENCHMARK_REQUESTED_REF",
    "AGP_BENCHMARK_NOTES",
    "AGP_BENCHMARK_HARNESS",
    "AGP_BENCHMARK_DIRTY",
    "AGP_BENCHMARK_WARMUP_FRAMES",
    "AGP_BENCHMARK_SAMPLE_FRAMES"
)
$savedEnvironment = @{}
foreach ($name in $benchmarkVariables) {
    $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, "Process")
}

try {
    $env:AGP_BENCHMARK = "1"
    $env:AGP_BENCHMARK_OUTPUT = $ResultsRoot
    $env:AGP_BENCHMARK_LABEL = $Label
    $env:AGP_BENCHMARK_COMMIT = $CommitOverride
    $env:AGP_BENCHMARK_BRANCH = $BranchOverride
    $env:AGP_BENCHMARK_CONFIGURATION = $Configuration
    $env:AGP_BENCHMARK_SCENARIO = $Scenario
    $env:AGP_BENCHMARK_COMPARISON_ID = $ComparisonId
    $env:AGP_BENCHMARK_REQUESTED_REF = $RequestedRef
    $env:AGP_BENCHMARK_NOTES = $Notes
    $env:AGP_BENCHMARK_HARNESS = $Harness
    $env:AGP_BENCHMARK_DIRTY = if ($Dirty) { "1" } else { "0" }
    $env:AGP_BENCHMARK_WARMUP_FRAMES = $WarmupFrames.ToString()
    $env:AGP_BENCHMARK_SAMPLE_FRAMES = $SampleFrames.ToString()

    for ($runOffset = 0; $runOffset -lt $Repetitions; $runOffset++) {
        $currentRunIndex = $RunIndex + $runOffset
        $currentExecutionIndex = $ExecutionIndex + $runOffset
        $env:AGP_BENCHMARK_RUN = $currentRunIndex.ToString()
        $env:AGP_BENCHMARK_EXECUTION_INDEX = $currentExecutionIndex.ToString()
        Write-Host "Running '$Label' repetition $currentRunIndex (execution $currentExecutionIndex, scenario $Scenario; $WarmupFrames warmup + $SampleFrames measured frames)..."
        $process = Start-Process -FilePath $executable -WorkingDirectory $RepoRoot -Wait -PassThru
        if ($process.ExitCode -ne 0) {
            throw "ModelViewer benchmark exited with code $($process.ExitCode)."
        }
    }
}
finally {
    foreach ($name in $benchmarkVariables) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], "Process")
    }
    if ($createdShaderJunction) {
        Remove-CreatedJunction -Path $runtimeShaderDirectory
    }
}

if (-not $NoReport) {
    $reportScript = Join-Path $PSScriptRoot "generate_report.py"
    $reportPath = Join-Path $ResultsRoot "report.html"
    $python = Get-Command python -ErrorAction SilentlyContinue
    if (-not $python) {
        $python = Get-Command py -ErrorAction SilentlyContinue
    }
    if ($python) {
        & $python.Source $reportScript --results $ResultsRoot --output $reportPath
        if ($LASTEXITCODE -ne 0) {
            throw "Report generation failed with exit code $LASTEXITCODE."
        }
        Write-Host "Report: $reportPath"
    }
    else {
        Write-Warning "Python was not found; results were saved, but report.html was not regenerated."
    }
}

Write-Host "Benchmark results: $ResultsRoot"
