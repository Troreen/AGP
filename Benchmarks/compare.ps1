[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string[]]$Refs,
    [string]$HarnessCommit,
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [ValidateSet("default", "busy")]
    [string]$Scenario = "default",
    [ValidateRange(1, 1000000)]
    [int]$WarmupFrames = 300,
    [ValidateRange(1, 10000000)]
    [int]$SampleFrames = 1200,
    [ValidateRange(1, 20)]
    [int]$Repetitions = 10,
    [string]$ComparisonId,
    [string]$Notes = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$resultsRoot = Join-Path $repoRoot "Benchmarks\results"
$runScript = Join-Path $PSScriptRoot "run.ps1"

function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [switch]$ReturnText,
        [switch]$AllowFailure
    )

    $output = & git -C $WorkingDirectory @Arguments
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0 -and -not $AllowFailure) {
        throw "git $($Arguments -join ' ') failed with exit code $exitCode."
    }
    if ($ReturnText) {
        return (($output | Out-String).Trim())
    }
    return $exitCode
}

if ($Refs.Count -lt 2) {
    throw "Provide at least two refs for a comparison."
}
if (-not $ComparisonId) {
    $timestamp = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmssZ")
    $ComparisonId = $timestamp + "-" + [Guid]::NewGuid().ToString("N").Substring(0, 8)
}
if ($ComparisonId -notmatch '^[A-Za-z0-9._-]+$') {
    throw "ComparisonId may contain only letters, numbers, periods, underscores, and hyphens."
}

if (-not $HarnessCommit) {
    $HarnessCommit = Invoke-Git -WorkingDirectory $repoRoot -Arguments @(
        "log", "-1", "--format=%H", "--", "Source/Utilities/FrameBenchmark.cpp"
    ) -ReturnText
}
if (-not $HarnessCommit) {
    throw "The benchmark harness is not committed yet. Commit it as one focused commit, then rerun this command or pass -HarnessCommit explicitly."
}
$HarnessCommit = Invoke-Git -WorkingDirectory $repoRoot -Arguments @("rev-parse", "$HarnessCommit^{commit}") -ReturnText
$harnessShort = $HarnessCommit.Substring(0, 8)
$harnessStartHistory = & git -C $repoRoot log --reverse --format=%H $HarnessCommit -- "Source/Utilities/FrameBenchmark.cpp"
if ($LASTEXITCODE -ne 0 -or -not $harnessStartHistory) {
    throw "Commit $HarnessCommit does not contain the benchmark recorder history."
}
$harnessStartCommit = @($harnessStartHistory)[0]
$instrumentationPaths = @(
    "Bin/Release/libfbxsdk.dll",
    "Source/Utilities/FrameBenchmark.cpp",
    "Source/Utilities/FrameBenchmark.h",
    "Source/Graphics/GraphicsEngine/RHI/RenderHardwareInterface.cpp",
    "Source/Graphics/GraphicsEngine/RHI/RenderHardwareInterface.h",
    "Source/Graphics/GraphicsEngine/GraphicsEngine.vcxproj",
    "Source/Graphics/GraphicsEngine/GraphicsEngine.vcxproj.filters"
)

$systemTempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$tempBase = Join-Path $systemTempRoot ("agp-benchmark-" + [Guid]::NewGuid().ToString("N"))
$tempBase = [System.IO.Path]::GetFullPath($tempBase)
if (-not $tempBase.StartsWith($systemTempRoot, [StringComparison]::OrdinalIgnoreCase) -or
    -not (Split-Path -Leaf $tempBase).StartsWith("agp-benchmark-")) {
    throw "Refusing to use an unexpected temporary worktree path: $tempBase"
}
New-Item -ItemType Directory -Force -Path $tempBase | Out-Null

$harnessPatchPath = Join-Path $tempBase "benchmark-harness.patch"
& git -C $repoRoot diff --binary "--output=$harnessPatchPath" "$harnessStartCommit^" $HarnessCommit -- @instrumentationPaths
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $harnessPatchPath) -or
    (Get-Item -LiteralPath $harnessPatchPath).Length -eq 0) {
    throw "Could not extract the benchmark instrumentation through commit $HarnessCommit."
}

$entries = @()
try {
    for ($refIndex = 0; $refIndex -lt $Refs.Count; $refIndex++) {
        $requestedRef = $Refs[$refIndex]
        $commit = Invoke-Git -WorkingDirectory $repoRoot -Arguments @("rev-parse", "$requestedRef^{commit}") -ReturnText
        $safeRef = $requestedRef -replace '[^A-Za-z0-9_-]', '-'
        $worktree = Join-Path $tempBase ((($refIndex + 1).ToString("00")) + "-" + $safeRef + "-" + $commit.Substring(0, 8))
        $worktree = [System.IO.Path]::GetFullPath($worktree)
        if (-not $worktree.StartsWith($tempBase + [System.IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to create a worktree outside the comparison directory: $worktree"
        }

        Write-Host "Preparing $requestedRef ($($commit.Substring(0, 8)))..."
        Invoke-Git -WorkingDirectory $repoRoot -Arguments @("worktree", "add", "--detach", $worktree, $commit) | Out-Null
        $entry = [PSCustomObject]@{
            RequestedRef = $requestedRef
            Commit = $commit
            Worktree = $worktree
            Harness = ""
        }
        $entries += $entry

        $containsHarness = (Invoke-Git -WorkingDirectory $worktree -Arguments @(
            "merge-base", "--is-ancestor", $HarnessCommit, $commit
        ) -AllowFailure) -eq 0
        if ($containsHarness) {
            $entry.Harness = "in-tree:$harnessShort"
        }
        else {
            Write-Host "Applying benchmark-only instrumentation from $harnessShort without changing the measured ref..."
            & git -C $worktree apply --whitespace=nowarn $harnessPatchPath
            if ($LASTEXITCODE -ne 0) {
                throw "The benchmark instrumentation did not apply cleanly to $requestedRef."
            }
            $entry.Harness = "temporary:$harnessShort"
        }
    }

    foreach ($entry in $entries) {
        Write-Host "Building $($entry.RequestedRef) once..."
        $buildArguments = @{
            Configuration = $Configuration
            RepoRoot = $entry.Worktree
            BuildOnly = $true
        }
        & $runScript @buildArguments
    }

    $executionIndex = 0
    for ($roundIndex = 0; $roundIndex -lt $Repetitions; $roundIndex++) {
        for ($slotIndex = 0; $slotIndex -lt $entries.Count; $slotIndex++) {
            $entryIndex = ($roundIndex + $slotIndex) % $entries.Count
            $entry = $entries[$entryIndex]
            $executionIndex++
            $runIndex = $roundIndex + 1
            Write-Host "Comparison $ComparisonId execution $executionIndex`: $($entry.RequestedRef) run $runIndex/$Repetitions"
            $runArguments = @{
                Label = $entry.RequestedRef
                Configuration = $Configuration
                Scenario = $Scenario
                ComparisonId = $ComparisonId
                ExecutionIndex = $executionIndex
                RunIndex = $runIndex
                RequestedRef = $entry.RequestedRef
                WarmupFrames = $WarmupFrames
                SampleFrames = $SampleFrames
                Repetitions = 1
                Notes = $Notes
                RepoRoot = $entry.Worktree
                ResultsRoot = $resultsRoot
                CommitOverride = $entry.Commit
                BranchOverride = $entry.RequestedRef
                Harness = $entry.Harness
                DirtyOverride = $false
                NoBuild = $true
                NoReport = $true
            }
            & $runScript @runArguments
        }
    }
}
finally {
    foreach ($entry in $entries) {
        if ($entry.Worktree -and (Test-Path -LiteralPath $entry.Worktree)) {
            Invoke-Git -WorkingDirectory $repoRoot -Arguments @("worktree", "remove", "--force", $entry.Worktree) -AllowFailure | Out-Null
        }
    }
    if ($tempBase.StartsWith($systemTempRoot, [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $tempBase).StartsWith("agp-benchmark-") -and
        (Test-Path -LiteralPath $tempBase)) {
        Remove-Item -LiteralPath $tempBase -Recurse -Force
    }
    Invoke-Git -WorkingDirectory $repoRoot -Arguments @("worktree", "prune") -AllowFailure | Out-Null
}

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $python) {
    throw "Python was not found. Runs were saved, but the report could not be generated."
}

$reportPath = Join-Path $resultsRoot "report.html"
& $python.Source (Join-Path $PSScriptRoot "generate_report.py") --results $resultsRoot --output $reportPath
if ($LASTEXITCODE -ne 0) {
    throw "Report generation failed with exit code $LASTEXITCODE."
}

Write-Host "Comparison $ComparisonId complete: $reportPath"
